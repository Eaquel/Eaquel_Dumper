// =============================================================================
// Dumper.cpp  —  Eaquel IL2CPP Dumper  (ReZygisk / Zygisk Modülü)
// Derleyici  : C++23  |  CMake 3.28+  |  AGP 9.1.1  |  Gradle 9.4.1
// Android API: min 30 (Android 11)  —  max 36 (Android 16)
// Mimari     : arm64-v8a / armeabi-v7a
//
// ═══════════════════════════════════════════════════════════════════════════
//  BOOTLOOP KORUMA MİMARİSİ  (4 Katmanlı)
// ───────────────────────────────────────────────────────────────────────────
//  SORUN:
//    Zygisk/ReZygisk modülü, cihaz açılırken Zygote sürecine yüklenir.
//    JNI_OnLoad otomatik tetiklenir ve hackStart() çağrısı Zygote'un
//    içinde libil2cpp.so aramaya, bellek taramaya ve mprotect ile bellek
//    korumalarını kırmaya çalışır. Zygote bir Unity oyunu olmadığından
//    SIGSEGV alır ve sistem yeniden başlatma döngüsüne (bootloop) girer.
//
//  ÇÖZÜM  —  4 Katman:
//    Katman 1: isZygoteProcess()   → Mevcut PID zygote/zygote64 ise çık
//    Katman 2: onLoad / preAppSpecialize → Sadece hedef pakette aktif ol
//    Katman 3: JNI_OnLoad güvenliği → Zygote'daysak anında çık
//    Katman 4: hackStart() timeout  → 30 sn içinde bulamazsa dur
// =============================================================================

// IL2CPP API fonksiyon pointer tanımları — Core.hpp tarafından kullanılır
#define DO_API(r, n, p)           r (*n) p = nullptr;
#define DO_API_NO_RETURN(r, n, p) r (*n) p = nullptr;
#include "Core.hpp"
#undef DO_API
#undef DO_API_NO_RETURN

#include "ReZygisk.hpp"

#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <atomic>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <jni.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <sys/eventfd.h>
#include <linux/unistd.h>
#include <unistd.h>
#include <array>
#include <signal.h>
#include <link.h>
#include <mutex>
#include <poll.h>

// ─────────────────────────────────────────────────────────────────────────────
// Yardımcı: nullptr veya boş string için fallback döner
// ─────────────────────────────────────────────────────────────────────────────
static inline const char* safeStr(const char* s, const char* fallback = "") {
    return (s && *s) ? s : fallback;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fonksiyon ön bildirimleri  (forward declarations)
// ─────────────────────────────────────────────────────────────────────────────
static void runApiInit(uintptr_t base);
static void runDump(const std::string& out_dir, const config::DumperConfig& cfg);

// ─────────────────────────────────────────────────────────────────────────────
// Erken-hook paylaşım değişkenleri
// dlopen hook kurulmadan önce postAppSpecialize çıktı dizinini saklar
// ─────────────────────────────────────────────────────────────────────────────
static std::string          s_early_out_dir;          // Erken hook çıktı dizini
static config::DumperConfig s_early_cfg;              // Erken hook konfigürasyonu
static std::atomic<bool>    s_early_hook_fired{false};// Erken hook bir kez tetiklensin
static std::mutex           s_early_cfg_mutex;        // Erken cfg mutex

// =============================================================================
//  KATMAN 1 — isZygoteProcess()
//  /proc/self/cmdline okuyarak mevcut process'in Zygote olup olmadığını
//  kontrol eder. Zygote ise hiçbir hack kodu çalışmamalıdır.
// =============================================================================
[[nodiscard]] static bool isZygoteProcess() noexcept {
    // /proc/self/cmdline → process adı null-terminated string olarak saklanır
    char cmdline[256] = {};
    int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        // Açamazsak güvenli tarafta kalıp Zygote olduğunu varsay
        LOGW("[BootGuard] cmdline okunamadı errno=%d — Zygote olarak kabul ediliyor", errno);
        return true;
    }
    ssize_t n = read(fd, cmdline, sizeof(cmdline) - 1);
    close(fd);
    if (n <= 0) return true;                      // Okuma başarısız → güvenli taraf

    // "zygote" veya "zygote64" ile başlıyorsa bu process Zygote'tur
    bool is_zygote = (strncmp(cmdline, "zygote", 6) == 0);
    if (is_zygote) {
        LOGW("[BootGuard] ZYGOTE DETECTED — cmdline='%s' — hack devre dışı!", cmdline);
    }
    return is_zygote;
}

// =============================================================================
//  hook_engine namespace
//  Inline hook ve GOT patch altyapısı — yalnızca hedef uygulama sürecinde
//  çağrılır (preAppSpecialize ile güvence altındadır).
// =============================================================================
namespace hook_engine {

static constexpr size_t kPageSize = 4096;

// Sayfa hizalama yardımcıları
static uintptr_t alignDown(uintptr_t a) { return a & ~(kPageSize - 1u); }
static uintptr_t alignUp  (uintptr_t a) { return (a + kPageSize - 1u) & ~(kPageSize - 1u); }

// ─────────────────────────────────────────────────────────────────────────────
// stealthPatchWindow: Bellek korumasını geçici olarak RW yapıp patch uygular,
// ardından RX olarak geri yükler. Zygote sürecinde çağrılmaz.
// ─────────────────────────────────────────────────────────────────────────────
static bool stealthPatchWindow(uintptr_t addr, size_t len,
                               const std::function<void()>& patch_fn)
{
    void*  page     = reinterpret_cast<void*>(alignDown(addr));
    size_t page_len = alignUp(len + (addr - alignDown(addr)));

    // RW koruma aç
    if (mprotect(page, page_len, PROT_READ | PROT_WRITE) != 0) {
        LOGE("hook: mprotect RW failed addr=0x%" PRIxPTR " errno=%d", addr, errno);
        return false;
    }
    patch_fn();

    // RX korumaya geri dön
    if (mprotect(page, page_len, PROT_READ | PROT_EXEC) != 0) {
        LOGE("hook: mprotect RX failed addr=0x%" PRIxPTR " errno=%d", addr, errno);
        return false;
    }
    return true;
}

// I-cache temizle — hook yazıldıktan sonra CPU'nun eski kodu görmemesi için
static void flushCache(uintptr_t addr, size_t len) {
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr + len));
}

// ─────────────────────────────────────────────────────────────────────────────
// allocTrampolinePage: Hedef adrese ±128 MB yakın boş bir sayfa bul ve ayır.
// Bulamazsa MAP_ANONYMOUS ile herhangi bir yere ayır.
// ─────────────────────────────────────────────────────────────────────────────
static void* allocTrampolinePage(uintptr_t near_addr) {
    uintptr_t lo = (near_addr > 0x8000000u) ? (near_addr - 0x8000000u) : 0;
    uintptr_t hi = near_addr + 0x8000000u;

    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return nullptr;

    uintptr_t prev_end = lo;
    void*     result   = nullptr;
    char      line[256];

    while (fgets(line, sizeof(line), f)) {
        uintptr_t s = 0, e = 0;
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &s, &e) != 2) continue;
        if (s > hi) break;
        if (s > prev_end && (s - prev_end) >= kPageSize) {
            uintptr_t try_addr = (prev_end + kPageSize - 1) & ~(kPageSize - 1u);
            if (try_addr + kPageSize <= s) {
                void* p = mmap(reinterpret_cast<void*>(try_addr), kPageSize,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                               -1, 0);
                if (p != MAP_FAILED) { result = p; break; }
            }
        }
        prev_end = e;
    }
    fclose(f);

    // Yakın yer bulunamazsa herhangi bir adrese yerleştir
    if (!result) {
        result = mmap(nullptr, kPageSize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (result == MAP_FAILED) result = nullptr;
    }
    return result;
}

// Trampolin sayfasını çalıştırılabilir yap
static bool lockTrampolineRX(void* page) {
    return mprotect(page, kPageSize, PROT_READ | PROT_EXEC) == 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// AArch64 (arm64-v8a) inline hook implementasyonu
// LDR x17, #8  +  BR x17  +  64-bit hedef adres (16 byte toplam)
// ─────────────────────────────────────────────────────────────────────────────
#if defined(__aarch64__)
static constexpr size_t kHookSize = 16;

struct Trampoline {
    uint8_t  saved[kHookSize]; // Orijinal ilk 16 byte
    uint32_t ldr_x17;          // LDR x17, #8
    uint32_t br_x17;           // BR  x17
    uint64_t return_addr;      // Orijinal fonksiyon + 16 (hook sonrası devam)
};

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) {
    auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[arm64]: trampoline alloc failed"); return false; }

    // Trampolin: orijinal baytları koru + orijinal+16 adresine zıpla
    auto* tramp       = reinterpret_cast<Trampoline*>(tramp_page);
    memcpy(tramp->saved, reinterpret_cast<void*>(tgt), kHookSize);
    tramp->ldr_x17    = 0x58000051u; // LDR x17, #8
    tramp->br_x17     = 0xD61F0220u; // BR  x17
    tramp->return_addr = static_cast<uint64_t>(tgt + kHookSize);
    flushCache(reinterpret_cast<uintptr_t>(tramp), sizeof(Trampoline));

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[arm64]: trampoline lock RX failed"); return false;
    }

    // Hedef fonksiyonun başına hook yönlendirmesini yaz
    bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        auto*    dst      = reinterpret_cast<uint8_t*>(tgt);
        uint32_t ldr      = 0x58000051u;
        uint32_t br       = 0xD61F0220u;
        memcpy(dst,     &ldr, 4);
        memcpy(dst + 4, &br,  4);
        uint64_t hook_addr = reinterpret_cast<uint64_t>(hook);
        memcpy(dst + 8, &hook_addr, 8);
        flushCache(tgt, kHookSize);
    });

    if (!ok) return false;
    if (orig_out) *orig_out = reinterpret_cast<void*>(tramp);
    LOGI("hook[arm64]: target=%p hook=%p tramp=%p", target, hook, tramp);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ARMv7 / Thumb-2 (armeabi-v7a) inline hook implementasyonu
// LDR PC, [PC+0]  +  hook_addr (8 byte toplam, Thumb modunda)
// ─────────────────────────────────────────────────────────────────────────────
#elif defined(__arm__)
static constexpr size_t kHookSize = 8;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) {
    auto tgt_thumb = reinterpret_cast<uintptr_t>(target);
    auto tgt       = tgt_thumb & ~1u; // Thumb bitini temizle
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[arm32]: trampoline alloc failed"); return false; }

    // Orijinal 8 byte'ı trampoline'e kopyala, ardından geri dön
    memcpy(tramp_page, reinterpret_cast<void*>(tgt), kHookSize);
    const uint8_t jback[] = { 0xDF, 0xF8, 0x00, 0xF0 }; // LDR PC, [PC+0]
    memcpy(tramp_page + kHookSize, jback, 4);
    uint32_t ret_addr = static_cast<uint32_t>(tgt + kHookSize) | 1u; // Thumb bit
    memcpy(tramp_page + kHookSize + 4, &ret_addr, 4);
    flushCache(reinterpret_cast<uintptr_t>(tramp_page), kHookSize + 8);

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[arm32]: trampoline lock RX failed"); return false;
    }

    bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        const uint8_t ldr_pc[] = { 0xDF, 0xF8, 0x00, 0xF0 }; // LDR PC, [PC+0]
        memcpy(reinterpret_cast<void*>(tgt), ldr_pc, 4);
        uint32_t hook_addr = reinterpret_cast<uint32_t>(hook) | 1u;
        memcpy(reinterpret_cast<void*>(tgt + 4), &hook_addr, 4);
        flushCache(tgt, kHookSize);
    });

    if (!ok) return false;
    if (orig_out) *orig_out = reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(tramp_page) | 1u);
    LOGI("hook[arm32]: target=%p hook=%p tramp=%p", target, hook, tramp_page);
    return true;
}

#else
// Desteklenmeyen mimari — hook yok
[[nodiscard]] bool installInlineHook(void*, void*, void**) { return false; }
#endif

// ─────────────────────────────────────────────────────────────────────────────
// patchGotSlot: GOT (Global Offset Table) içindeki fonksiyon slotunu hookla.
// PLT/GOT hook, inline hook'tan daha az risklidir ancak sadece kendi .so için.
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] bool patchGotSlot(void* target_fn, void* hook_fn, void** orig_out) {
    Dl_info di{};
    if (!dladdr(reinterpret_cast<void*>(&patchGotSlot), &di) || !di.dli_fbase) {
        LOGE("GOT patch: dladdr failed"); return false;
    }
    auto self_base = reinterpret_cast<uintptr_t>(di.dli_fbase);

    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return false;
    char line[512];

    while (fgets(line, sizeof(line), maps)) {
        uintptr_t seg_s = 0, seg_e = 0;
        char perms[8] = {};
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %7s", &seg_s, &seg_e, perms) < 3) continue;
        if (perms[1] != 'w') continue;                                  // Yazılabilir segment
        if (seg_s < self_base || seg_s > self_base + 0x8000000u) continue; // Kendi .so sınırı

        for (uintptr_t addr = seg_s; addr + sizeof(void*) <= seg_e; addr += sizeof(void*)) {
            void** slot = reinterpret_cast<void**>(addr);
            if (*slot != target_fn) continue;
            uintptr_t page = alignDown(addr);
            if (mprotect(reinterpret_cast<void*>(page), kPageSize, PROT_READ | PROT_WRITE) != 0)
                continue;
            if (orig_out) *orig_out = *slot;
            *slot = hook_fn;
            mprotect(reinterpret_cast<void*>(page), kPageSize, PROT_READ);
            LOGI("GOT patch: slot=0x%" PRIxPTR " new=%p", addr, hook_fn);
            fclose(maps);
            return true;
        }
    }
    fclose(maps);
    LOGW("GOT patch: slot for %p not found", target_fn);
    return false;
}

} // namespace hook_engine

// =============================================================================
//  SIGSEGV handler — crash'leri yakalar ve loga yazar, Zygote'u öldürmez
//  Sadece hedef uygulama sürecinde kurulur.
// =============================================================================
static void installSigsegvHandler() {
    struct sigaction sa{};
    sa.sa_sigaction = scanner::sigsegv_handler;
    sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
}

// =============================================================================
//  metadata_hook namespace
//  IL2CPP metadata loader fonksiyonunu hooklar; şifreli metadata'yı yakalar,
//  XOR anahtarını tahmin ederek çözmeye çalışır ve diske yazar.
// =============================================================================
namespace metadata_hook {

using MetadataLoadFn = void* (*)(const char* path, size_t* out_size);

static MetadataLoadFn    s_orig_load = nullptr;
static std::atomic<bool> s_fired{false};         // Bir kez tetiklensin
static std::string       s_dump_dir;
static std::mutex        s_mutex;

// ─────────────────────────────────────────────────────────────────────────────
// persistDecryptedMetadata: Bellekteki metadata buffer'ını analiz eder,
// şifreli ise XOR key bularak çözer ve diske yazar.
// ─────────────────────────────────────────────────────────────────────────────
static void persistDecryptedMetadata(const void* data, size_t size, const char* src_path) {
    if (!data || size < 4) return;

    auto* buf   = reinterpret_cast<const uint8_t*>(data);
    auto  state = entropy::analyzeBuffer(buf, size);
    std::vector<uint8_t> plaintext;

    if (state == entropy::MetadataState::Encrypted) {
        LOGI("[MetaHook] Buffer şifreli — XOR anahtar taraması başlıyor");
        auto key = entropy::discoverXorKey(buf, size);
        if (key.found && key.score >= 2) {
            plaintext = entropy::decryptBuffer(buf, size, key);
            LOGI("[MetaHook] Şifre çözme tamamlandı key_len=%zu score=%d",
                 key.key_len, key.score);
        } else {
            LOGW("[MetaHook] XOR anahtarı doğrulanamadı — ham buffer yazılıyor");
            plaintext.assign(buf, buf + size);
        }
    } else {
        // Zaten plain — direkt kopyala
        plaintext.assign(buf, buf + size);
    }

    // Magic byte doğrulama
    uint32_t out_magic = 0;
    if (plaintext.size() >= 4) memcpy(&out_magic, plaintext.data(), 4);
    if (out_magic != entropy::kIl2CppMetadataMagic)
        LOGW("[MetaHook] Çıkış magic 0x%08X — hâlâ şifreli olabilir", out_magic);

    // Çıktı dizinini oluştur ve dosyayı yaz
    std::lock_guard<std::mutex> lk(s_mutex);
    struct stat st{};
    if (stat(s_dump_dir.c_str(), &st) != 0) mkdir(s_dump_dir.c_str(), 0777);

    std::string out_path = s_dump_dir + "/global-metadata.dat";
    int fd = open(out_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        LOGE("[MetaHook] open(%s) başarısız errno=%d", out_path.c_str(), errno);
        return;
    }
    const uint8_t* ptr  = plaintext.data();
    size_t         left = plaintext.size();
    while (left > 0) {
        ssize_t w = write(fd, ptr, left);
        if (w <= 0) break;
        ptr  += w;
        left -= static_cast<size_t>(w);
    }
    close(fd);
    LOGI("[MetaHook] metadata yazıldı (%zu byte) %s -> %s",
         plaintext.size(), src_path ? src_path : "?", out_path.c_str());
}

// Hooklanmış metadata loader — orijinalini çağır, sonucu yakala
static void* hooked_MetadataLoad(const char* path, size_t* out_size) {
    void* result = s_orig_load(path, out_size);
    bool  expected = false;
    if (result && out_size && s_fired.compare_exchange_strong(expected, true)) {
        LOGI("[MetaHook] tetiklendi path=%s size=%zu", path ? path : "?", *out_size);
        std::thread([data  = result,
                     size  = *out_size,
                     spath = std::string(path ? path : "")]() {
            persistDecryptedMetadata(data, size, spath.c_str());
        }).detach();
    }
    return result;
}

// Hook'u ilgili IL2CPP sembolüne kur
static bool install(uintptr_t lib_base, const std::string& dump_dir) {
    {
        std::lock_guard<std::mutex> lk(s_mutex);
        s_dump_dir = dump_dir;
    }
    auto      syms        = scanner::scanAllSymbols(lib_base);
    uintptr_t loader_addr = 0;

#if defined(__aarch64__)
    loader_addr = syms.metadata_cache_initialize
                  ? syms.metadata_cache_initialize
                  : syms.metadata_loader;
#elif defined(__arm__)
    loader_addr = syms.arm32_metadata_cache_init
                  ? syms.arm32_metadata_cache_init
                  : syms.arm32_metadata_loader;
#endif

    if (!loader_addr) { LOGW("[MetaHook] loader adresi bulunamadı"); return false; }

    bool ok = hook_engine::installInlineHook(
        reinterpret_cast<void*>(loader_addr),
        reinterpret_cast<void*>(&hooked_MetadataLoad),
        reinterpret_cast<void**>(&s_orig_load)
    );

    if (ok) LOGI("[MetaHook] inline hook kuruldu 0x%" PRIxPTR, loader_addr);
    else    LOGE("[MetaHook] inline hook başarısız");
    return ok;
}

// Hot-reload için hook durumunu sıfırla
static void resetForReload(const std::string& new_dump_dir) {
    std::lock_guard<std::mutex> lk(s_mutex);
    s_dump_dir = new_dump_dir;
    s_fired.store(false, std::memory_order_release);
    LOGI("[MetaHook] hot-reload sıfırlandı dump_dir=%s", new_dump_dir.c_str());
}

} // namespace metadata_hook

// =============================================================================
//  dlopen hook — libil2cpp.so yüklendiği anda yakalamak için
//  Protected_Breaker aktifse postAppSpecialize'da kurulur.
//  SADECE hedef uygulama sürecinde çalışır.
// =============================================================================
static void* (*s_orig_dlopen)(const char*, int) = nullptr;

static void* hooked_dlopen(const char* path, int flags) {
    void* handle = s_orig_dlopen(path, flags);
    if (handle && path && strstr(path, "libil2cpp.so")) {
        bool expected = false;
        if (s_early_hook_fired.compare_exchange_strong(expected, true)) {
            LOGI("EARLY HOOK: libil2cpp.so yüklendi path=%s", path);

            std::string out;
            config::DumperConfig cfg;
            {
                std::lock_guard<std::mutex> lk(s_early_cfg_mutex);
                out = s_early_out_dir;
                cfg = s_early_cfg;
            }

            std::thread([out = std::move(out), cfg = std::move(cfg), handle]() mutable {
                // Kütüphanenin tam yüklenmesi için kısa bekleme (~30 ms)
                int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
                if (efd >= 0) {
                    struct pollfd pf = { efd, POLLIN, 0 };
                    poll(&pf, 1, 30); // 30 ms yeterli
                    ::close(efd);
                }

                // Base adresi bul
                uintptr_t base = 0;
                Dl_info   di{};
                if (dladdr(handle, &di) && di.dli_fbase)
                    base = reinterpret_cast<uintptr_t>(di.dli_fbase);
                if (!base)
                    base = scanner::findLibBase("libil2cpp.so");

                if (base) {
                    if (cfg.Protected_Breaker)
                        metadata_hook::install(base, out);
                    runApiInit(base);
                }
                runDump(out, cfg);
            }).detach();
        }
    }
    return handle;
}

// dlopen hook'unu kur — inline hook dene, başarısız olursa GOT patch yap
static bool installDlopenHook(const std::string& out_dir,
                               const config::DumperConfig& cfg)
{
    {
        std::lock_guard<std::mutex> lk(s_early_cfg_mutex);
        s_early_out_dir = out_dir;
        s_early_cfg     = cfg;
    }
    s_early_hook_fired.store(false);

    if (hook_engine::installInlineHook(
            reinterpret_cast<void*>(dlopen),
            reinterpret_cast<void*>(hooked_dlopen),
            reinterpret_cast<void**>(&s_orig_dlopen)))
    {
        LOGI("dlopen hook kuruldu (inline hook)");
        return true;
    }

    // Inline hook başarısız → GOT patch dene
    if (hook_engine::patchGotSlot(
            reinterpret_cast<void*>(dlopen),
            reinterpret_cast<void*>(hooked_dlopen),
            reinterpret_cast<void**>(&s_orig_dlopen)))
    {
        LOGI("dlopen hook kuruldu (GOT patch)");
        s_orig_dlopen = dlopen; // Güvenli fallback
        return true;
    }

    LOGW("dlopen hook başarısız — libil2cpp.so polling moduna geçildi");
    return false;
}

// =============================================================================
//  il2cpp_api namespace — IL2CPP fonksiyon pointer tablosu
//  Semboller dlopen + pattern scan ile doldurulur.
// =============================================================================
namespace il2cpp_api {

struct FunctionTable {
    // Temel domain/assembly API
    bool           (*init)(const char*, int)                                             = nullptr;
    Il2CppDomain*  (*domain_get)()                                                       = nullptr;
    const Il2CppAssembly** (*domain_get_assemblies)(const Il2CppDomain*, size_t*)        = nullptr;
    const Il2CppImage* (*assembly_get_image)(const Il2CppAssembly*)                      = nullptr;
    const char*    (*image_get_name)(const Il2CppImage*)                                 = nullptr;
    size_t         (*image_get_class_count)(const Il2CppImage*)                          = nullptr;
    Il2CppClass*   (*image_get_class)(const Il2CppImage*, size_t)                        = nullptr;

    // Sınıf API
    const Il2CppType* (*class_get_type)(Il2CppClass*)                                   = nullptr;
    Il2CppClass*   (*class_from_type)(const Il2CppType*)                                 = nullptr;
    const char*    (*class_get_name)(Il2CppClass*)                                       = nullptr;
    const char*    (*class_get_namespace)(Il2CppClass*)                                  = nullptr;
    uint32_t       (*class_get_flags)(Il2CppClass*)                                      = nullptr;
    bool           (*class_is_valuetype)(Il2CppClass*)                                   = nullptr;
    bool           (*class_is_enum)(Il2CppClass*)                                        = nullptr;
    bool           (*class_is_abstract)(Il2CppClass*)                                    = nullptr;
    bool           (*class_is_interface)(Il2CppClass*)                                   = nullptr;
    bool           (*class_is_generic)(Il2CppClass*)                                     = nullptr;
    bool           (*class_is_inflated)(Il2CppClass*)                                    = nullptr;
    Il2CppClass*   (*class_get_parent)(Il2CppClass*)                                     = nullptr;
    Il2CppClass*   (*class_get_interfaces)(Il2CppClass*, void**)                         = nullptr;
    Il2CppClass*   (*class_get_nested_types)(Il2CppClass*, void**)                       = nullptr;
    FieldInfo*     (*class_get_fields)(Il2CppClass*, void**)                             = nullptr;
    PropertyInfo*  (*class_get_properties)(Il2CppClass*, void**)                         = nullptr;
    const MethodInfo* (*class_get_methods)(Il2CppClass*, void**)                         = nullptr;
    const MethodInfo* (*class_get_method_from_name)(Il2CppClass*, const char*, int)      = nullptr;
    Il2CppClass*   (*class_from_name)(const Il2CppImage*, const char*, const char*)      = nullptr;
    Il2CppClass*   (*class_from_system_type)(Il2CppReflectionType*)                      = nullptr;
    const Il2CppImage* (*get_corlib)()                                                   = nullptr;

    // Alan (Field) API
    uint32_t       (*field_get_flags)(FieldInfo*)                                        = nullptr;
    const char*    (*field_get_name)(FieldInfo*)                                         = nullptr;
    size_t         (*field_get_offset)(FieldInfo*)                                       = nullptr;
    const Il2CppType* (*field_get_type)(FieldInfo*)                                      = nullptr;
    void           (*field_static_get_value)(FieldInfo*, void*)                          = nullptr;

    // Özellik (Property) API
    uint32_t       (*property_get_flags)(PropertyInfo*)                                  = nullptr;
    const MethodInfo* (*property_get_get_method)(PropertyInfo*)                          = nullptr;
    const MethodInfo* (*property_get_set_method)(PropertyInfo*)                          = nullptr;
    const char*    (*property_get_name)(PropertyInfo*)                                   = nullptr;

    // Metot API
    const Il2CppType* (*method_get_return_type)(const MethodInfo*)                       = nullptr;
    const char*    (*method_get_name)(const MethodInfo*)                                 = nullptr;
    uint32_t       (*method_get_param_count)(const MethodInfo*)                          = nullptr;
    const Il2CppType* (*method_get_param)(const MethodInfo*, uint32_t)                   = nullptr;
    uint32_t       (*method_get_flags)(const MethodInfo*, uint32_t*)                     = nullptr;
    const char*    (*method_get_param_name)(const MethodInfo*, uint32_t)                 = nullptr;

    // Tip / Thread API
    bool           (*type_is_byref)(const Il2CppType*)                                   = nullptr;
    Il2CppThread*  (*thread_attach)(Il2CppDomain*)                                       = nullptr;
    bool           (*is_vm_thread)(Il2CppThread*)                                        = nullptr;
    Il2CppString*  (*string_new)(const char*)                                            = nullptr;
};

static FunctionTable g_api;
static uint64_t      g_il2cpp_base = 0;

// Sembol çözücü — başarısız olursa LOGW yaz (crash değil)
template<typename T>
static void resolveSymbol(void* handle, const char* name, T& out) {
    if (!handle || !name) return;
    void* sym = dlsym(handle, name);
    if (sym) out = reinterpret_cast<T>(sym);
    else     LOGW("symbol çözülemedi: %s", name);
}

// Pattern scan fallback
template<typename T>
static void applyFallback(uintptr_t addr, T& out) {
    if (!out && addr && scanner::isReadableAddress(addr)) {
        out = reinterpret_cast<T>(addr);
        LOGI("scanner fallback: 0x%" PRIxPTR, addr);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// populateFunctionTable: IL2CPP sembollerini dlsym + pattern scan ile doldur
// ─────────────────────────────────────────────────────────────────────────────
static void populateFunctionTable(uintptr_t base) {
    // RTLD_NOLOAD → zaten yüklüyse handle al, yoklamak için yükleme
    void* handle = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
    if (!handle) LOGW("dlopen RTLD_NOLOAD başarısız — pattern scan moduna geçildi");

    if (handle) {
        auto r = [&](const char* name, auto& out) { resolveSymbol(handle, name, out); };

        // Temel API sembolleri
        r("il2cpp_init",                       g_api.init);
        r("il2cpp_domain_get",                 g_api.domain_get);
        r("il2cpp_domain_get_assemblies",      g_api.domain_get_assemblies);
        r("il2cpp_assembly_get_image",         g_api.assembly_get_image);
        r("il2cpp_image_get_name",             g_api.image_get_name);
        r("il2cpp_image_get_class_count",      g_api.image_get_class_count);
        r("il2cpp_image_get_class",            g_api.image_get_class);
        r("il2cpp_class_get_type",             g_api.class_get_type);
        r("il2cpp_class_from_type",            g_api.class_from_type);
        r("il2cpp_class_get_name",             g_api.class_get_name);
        r("il2cpp_class_get_namespace",        g_api.class_get_namespace);
        r("il2cpp_class_get_flags",            g_api.class_get_flags);
        r("il2cpp_class_is_valuetype",         g_api.class_is_valuetype);
        r("il2cpp_class_is_enum",              g_api.class_is_enum);
        r("il2cpp_class_is_abstract",          g_api.class_is_abstract);
        r("il2cpp_class_is_interface",         g_api.class_is_interface);
        r("il2cpp_class_is_generic",           g_api.class_is_generic);
        r("il2cpp_class_is_inflated",          g_api.class_is_inflated);
        r("il2cpp_class_get_parent",           g_api.class_get_parent);
        r("il2cpp_class_get_interfaces",       g_api.class_get_interfaces);
        r("il2cpp_class_get_nested_types",     g_api.class_get_nested_types);
        r("il2cpp_class_get_fields",           g_api.class_get_fields);
        r("il2cpp_class_get_properties",       g_api.class_get_properties);
        r("il2cpp_class_get_methods",          g_api.class_get_methods);
        r("il2cpp_class_get_method_from_name", g_api.class_get_method_from_name);
        r("il2cpp_class_from_name",            g_api.class_from_name);
        r("il2cpp_class_from_system_type",     g_api.class_from_system_type);
        r("il2cpp_get_corlib",                 g_api.get_corlib);
        r("il2cpp_field_get_flags",            g_api.field_get_flags);
        r("il2cpp_field_get_name",             g_api.field_get_name);
        r("il2cpp_field_get_offset",           g_api.field_get_offset);
        r("il2cpp_field_get_type",             g_api.field_get_type);
        r("il2cpp_field_static_get_value",     g_api.field_static_get_value);
        r("il2cpp_property_get_flags",         g_api.property_get_flags);
        r("il2cpp_property_get_get_method",    g_api.property_get_get_method);
        r("il2cpp_property_get_set_method",    g_api.property_get_set_method);
        r("il2cpp_property_get_name",          g_api.property_get_name);
        r("il2cpp_method_get_return_type",     g_api.method_get_return_type);
        r("il2cpp_method_get_name",            g_api.method_get_name);
        r("il2cpp_method_get_param_count",     g_api.method_get_param_count);
        r("il2cpp_method_get_param",           g_api.method_get_param);
        r("il2cpp_method_get_flags",           g_api.method_get_flags);
        r("il2cpp_method_get_param_name",      g_api.method_get_param_name);
        r("il2cpp_type_is_byref",              g_api.type_is_byref);
        r("il2cpp_thread_attach",              g_api.thread_attach);
        r("il2cpp_is_vm_thread",               g_api.is_vm_thread);
        r("il2cpp_string_new",                 g_api.string_new);

        // Mangled isim fallback — bazı Unity versiyonları C++ sembolü kullanır
        if (!g_api.domain_get) {
            void* s = dlsym(handle, "_ZN6il2cpp2vm6Domain3GetEv");
            if (s) g_api.domain_get = reinterpret_cast<decltype(g_api.domain_get)>(s);
        }
        if (!g_api.domain_get_assemblies) {
            void* s = dlsym(handle, "_ZN6il2cpp2vm6Domain12GetAssembliesEPKNS0_8Il2CppDomainEPm");
            if (s) g_api.domain_get_assemblies =
                reinterpret_cast<decltype(g_api.domain_get_assemblies)>(s);
        }
    }

    // Kritik semboller eksikse tam pattern scan yap
    const bool critical_missing =
        !g_api.domain_get || !g_api.domain_get_assemblies ||
        !g_api.image_get_class || !g_api.thread_attach || !g_api.is_vm_thread;

    if (critical_missing) {
        LOGW("Kritik semboller eksik — tam adaptif pattern scan başlıyor");
        auto s = scanner::scanAllSymbols(base);
        applyFallback(s.domain_get,                 g_api.domain_get);
        applyFallback(s.domain_get_assemblies,      g_api.domain_get_assemblies);
        applyFallback(s.image_get_class,            g_api.image_get_class);
        applyFallback(s.thread_attach,              g_api.thread_attach);
        applyFallback(s.is_vm_thread,               g_api.is_vm_thread);
        applyFallback(s.domain_get_2026,            g_api.domain_get);
        applyFallback(s.domain_get_assemblies_2024, g_api.domain_get_assemblies);
        applyFallback(s.image_get_class_2026,       g_api.image_get_class);
#if defined(__arm__)
        applyFallback(s.arm32_domain_get,            g_api.domain_get);
        applyFallback(s.arm32_domain_get_assemblies, g_api.domain_get_assemblies);
        applyFallback(s.arm32_image_get_class,       g_api.image_get_class);
        applyFallback(s.arm32_thread_attach,         g_api.thread_attach);
#endif
        if (!g_api.domain_get)            LOGE("FATAL: domain_get çözülemedi");
        if (!g_api.domain_get_assemblies) LOGE("FATAL: domain_get_assemblies çözülemedi");
        if (!g_api.image_get_class)       LOGW("image_get_class yok — reflection fallback");
    }
}

} // namespace il2cpp_api

// =============================================================================
//  Dump yardımcı fonksiyonları — IL2CPP metadatasını C++ header / Frida
//  script olarak diske yazar.
// =============================================================================

// Metot erişim modifier'ını string olarak döner
static std::string buildMethodModifier(uint32_t flags) {
    std::stringstream out;
    switch (flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK) {
        case METHOD_ATTRIBUTE_PRIVATE:       out << "private ";            break;
        case METHOD_ATTRIBUTE_PUBLIC:        out << "public ";             break;
        case METHOD_ATTRIBUTE_FAMILY:        out << "protected ";          break;
        case METHOD_ATTRIBUTE_ASSEM:
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM: out << "internal ";           break;
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM:  out << "protected internal "; break;
        default: break;
    }
    if (flags & METHOD_ATTRIBUTE_STATIC)   out << "static ";
    if (flags & METHOD_ATTRIBUTE_ABSTRACT) {
        out << "abstract ";
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            out << "override ";
    } else if (flags & METHOD_ATTRIBUTE_FINAL) {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            out << "sealed override ";
    } else if (flags & METHOD_ATTRIBUTE_VIRTUAL) {
        out << ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT
                ? "virtual " : "override ");
    }
    if (flags & METHOD_ATTRIBUTE_PINVOKE_IMPL) out << "extern ";
    return out.str();
}

// Null-safe sınıf adı
static std::string safeClassName(Il2CppClass* klass) {
    if (!klass) return "object";
    auto& api  = il2cpp_api::g_api;
    auto  name = api.class_get_name ? api.class_get_name(klass) : nullptr;
    return safeStr(name, "object");
}

// Metot yapısı
struct MethodEntry {
    std::string name, return_type, modifier;
    uint64_t    rva = 0, va = 0;
    std::vector<std::pair<std::string, std::string>> params; // {tip, isim}
};

// Alan (field) yapısı
struct FieldEntry {
    std::string name, type_name, modifier;
    uint64_t    offset = 0;
    bool        is_static = false;
};

// Özellik (property) yapısı
struct PropertyEntry {
    std::string name, type_name, modifier;
    bool        has_getter = false, has_setter = false;
};

// Sınıf yapısı
struct ClassEntry {
    std::string name, ns, parent, full_name;
    bool        is_abstract = false, is_interface = false,
                is_enum = false, is_valuetype = false,
                is_generic = false;
    std::vector<MethodEntry>   methods;
    std::vector<FieldEntry>    fields;
    std::vector<PropertyEntry> properties;
    std::vector<std::string>   interfaces;
};

// ─────────────────────────────────────────────────────────────────────────────
// Tip adını IL2CppType* → C# string olarak çözer
// ─────────────────────────────────────────────────────────────────────────────
static std::string resolveTypeName(const Il2CppType* t) {
    if (!t) return "void";
    auto& api = il2cpp_api::g_api;

    bool is_byref = api.type_is_byref && api.type_is_byref(t);
    Il2CppClass* klass = api.class_from_type ? api.class_from_type(t) : nullptr;
    std::string  base  = safeClassName(klass);

    // Jenerik parametre desteği
    if (klass && api.class_is_generic && api.class_is_generic(klass)) {
        base += "<>";
    } else if (klass && api.class_is_inflated && api.class_is_inflated(klass)) {
        base += "<T>";
    }

    // Dizi türleri
    if (t->type == IL2CPP_TYPE_SZARRAY || t->type == IL2CPP_TYPE_ARRAY) {
        base += "[]";
    }
    if (is_byref) base = "ref " + base;
    return base;
}

// ─────────────────────────────────────────────────────────────────────────────
// Bir sınıfın tüm metotlarını çıkarır
// ─────────────────────────────────────────────────────────────────────────────
static std::vector<MethodEntry> extractMethods(Il2CppClass* klass) {
    std::vector<MethodEntry> result;
    auto& api = il2cpp_api::g_api;
    if (!api.class_get_methods) return result;

    void*             iter   = nullptr;
    const MethodInfo* method = nullptr;
    while ((method = api.class_get_methods(klass, &iter)) != nullptr) {
        MethodEntry e;
        e.name = safeStr(api.method_get_name ? api.method_get_name(method) : nullptr, "?");

        uint32_t impl_flags = 0;
        uint32_t flags = api.method_get_flags ? api.method_get_flags(method, &impl_flags) : 0;
        e.modifier = buildMethodModifier(flags);

        const Il2CppType* rt = api.method_get_return_type
                               ? api.method_get_return_type(method) : nullptr;
        e.return_type = resolveTypeName(rt);

        uint32_t pcount = api.method_get_param_count
                          ? api.method_get_param_count(method) : 0;
        for (uint32_t i = 0; i < pcount; ++i) {
            const Il2CppType* pt  = api.method_get_param
                                    ? api.method_get_param(method, i) : nullptr;
            const char*       pn  = api.method_get_param_name
                                    ? api.method_get_param_name(method, i) : nullptr;
            e.params.emplace_back(resolveTypeName(pt), safeStr(pn, "arg"));
        }

        // RVA ve VA hesapla
        if (method->methodPointer) {
            e.va  = static_cast<uint64_t>(
                reinterpret_cast<uintptr_t>(method->methodPointer));
            e.rva = (il2cpp_api::g_il2cpp_base > 0)
                    ? (e.va - il2cpp_api::g_il2cpp_base)
                    : e.va;
        }
        result.push_back(std::move(e));
    }
    return result;
}

// Alanları çıkarır
static std::vector<FieldEntry> extractFields(Il2CppClass* klass) {
    std::vector<FieldEntry> result;
    auto& api = il2cpp_api::g_api;
    if (!api.class_get_fields) return result;

    void*      iter  = nullptr;
    FieldInfo* field = nullptr;
    while ((field = api.class_get_fields(klass, &iter)) != nullptr) {
        FieldEntry e;
        e.name = safeStr(api.field_get_name ? api.field_get_name(field) : nullptr, "?");
        const Il2CppType* ft = api.field_get_type ? api.field_get_type(field) : nullptr;
        e.type_name = resolveTypeName(ft);
        e.offset    = api.field_get_offset ? api.field_get_offset(field) : 0;
        uint32_t fg = api.field_get_flags ? api.field_get_flags(field) : 0;
        e.is_static = (fg & FIELD_ATTRIBUTE_STATIC) != 0;
        e.modifier  = e.is_static ? "static " : "";
        result.push_back(std::move(e));
    }
    return result;
}

// Özellikleri çıkarır
static std::vector<PropertyEntry> extractProperties(Il2CppClass* klass) {
    std::vector<PropertyEntry> result;
    auto& api = il2cpp_api::g_api;
    if (!api.class_get_properties) return result;

    void*         iter = nullptr;
    PropertyInfo* prop = nullptr;
    while ((prop = api.class_get_properties(klass, &iter)) != nullptr) {
        PropertyEntry e;
        e.name       = safeStr(api.property_get_name ? api.property_get_name(prop) : nullptr, "?");
        e.has_getter = api.property_get_get_method && api.property_get_get_method(prop);
        e.has_setter = api.property_get_set_method && api.property_get_set_method(prop);
        const MethodInfo* getter = e.has_getter
                                   ? api.property_get_get_method(prop) : nullptr;
        if (getter) {
            const Il2CppType* pt = api.method_get_return_type
                                   ? api.method_get_return_type(getter) : nullptr;
            e.type_name = resolveTypeName(pt);
        }
        result.push_back(std::move(e));
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// C++ header çıktısı — dump/{image_name}.h
// ─────────────────────────────────────────────────────────────────────────────
static void writeCppHeader(const std::string& out_dir,
                           const std::string& image_name,
                           const std::vector<ClassEntry>& classes)
{
    struct stat st{};
    if (stat(out_dir.c_str(), &st) != 0) mkdir(out_dir.c_str(), 0777);

    std::string path = out_dir + "/" + image_name + ".h";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("header yaz: %s açılamadı", path.c_str()); return; }

    f << "// Eaquel IL2CPP Dumper — Auto-generated C++ Header\n"
      << "// Image: " << image_name << "\n"
      << "#pragma once\n#include <cstdint>\n\n";

    for (auto& cls : classes) {
        if (!cls.ns.empty()) f << "// namespace " << cls.ns << "\n";
        if (cls.is_interface) f << "struct /* interface */ " << cls.name;
        else if (cls.is_enum) f << "enum class " << cls.name;
        else                  f << "struct " << cls.name;
        if (!cls.parent.empty() && cls.parent != "object")
            f << " : public " << cls.parent;
        f << " { // " << cls.full_name << "\n";

        // Alanlar
        for (auto& fld : cls.fields) {
            f << "    " << fld.modifier << fld.type_name << " " << fld.name
              << "; // offset=0x" << std::hex << fld.offset << std::dec << "\n";
        }
        // Metotlar
        for (auto& m : cls.methods) {
            f << "    " << m.modifier << m.return_type << " " << m.name << "(";
            for (size_t i = 0; i < m.params.size(); ++i) {
                if (i) f << ", ";
                f << m.params[i].first << " " << m.params[i].second;
            }
            f << "); // RVA: 0x" << std::hex << m.rva << std::dec << "\n";
        }
        f << "};\n\n";
    }
    LOGI("C++ header yazıldı: %s (%zu sınıf)", path.c_str(), classes.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Frida script çıktısı — dump/{image_name}.js
// ─────────────────────────────────────────────────────────────────────────────
static void writeFridaScript(const std::string& out_dir,
                             const std::string& image_name,
                             const std::vector<ClassEntry>& classes)
{
    struct stat st{};
    if (stat(out_dir.c_str(), &st) != 0) mkdir(out_dir.c_str(), 0777);

    std::string path = out_dir + "/" + image_name + ".js";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("frida script yaz: %s açılamadı", path.c_str()); return; }

    f << "// Eaquel IL2CPP Dumper — Auto-generated Frida Script\n"
      << "// Image: " << image_name << "\n"
      << "// Usage: frida -U -f com.example.app -l " << image_name << ".js\n\n"
      << "var il2cpp_base = Module.findBaseAddress('libil2cpp.so');\n"
      << "if (!il2cpp_base) throw new Error('libil2cpp.so not found');\n\n";

    for (auto& cls : classes) {
        for (auto& m : cls.methods) {
            if (!m.rva) continue;
            f << "// " << cls.full_name << "::" << m.name << "\n"
              << "var addr_" << cls.name << "_" << m.name
              << " = il2cpp_base.add(0x" << std::hex << m.rva << std::dec << ");\n"
              << "Interceptor.attach(addr_" << cls.name << "_" << m.name << ", {\n"
              << "    onEnter: function(args) {},\n"
              << "    onLeave: function(retval) {}\n"
              << "});\n\n";
        }
    }
    LOGI("Frida script yazıldı: %s (%zu sınıf)", path.c_str(), classes.size());
}

// =============================================================================
//  runApiInit — IL2CPP base'i ayarla ve fonksiyon tablosunu doldur
// =============================================================================
static void runApiInit(uintptr_t base) {
    il2cpp_api::g_il2cpp_base = static_cast<uint64_t>(base);
    il2cpp_api::populateFunctionTable(base);
}

// =============================================================================
//  runDump — Tüm image'ları tarayıp metadatayı diske yazar
// =============================================================================
static void runDump(const std::string& out_dir, const config::DumperConfig& cfg) {
    auto& api = il2cpp_api::g_api;
    if (!api.domain_get) {
        LOGE("runDump: domain_get null — dump iptal");
        return;
    }

    // IL2CPP domain'ini al
    Il2CppDomain* domain = api.domain_get();
    if (!domain) { LOGE("runDump: domain null"); return; }

    // Thread'i IL2CPP VM'e ekle (gerekli, aksi hâlde crash)
    if (api.thread_attach) api.thread_attach(domain);

    size_t count = 0;
    const Il2CppAssembly** assemblies = api.domain_get_assemblies
                                        ? api.domain_get_assemblies(domain, &count)
                                        : nullptr;
    if (!assemblies || !count) {
        LOGE("runDump: assembly listesi boş"); return;
    }

    LOGI("runDump: %zu assembly bulundu", count);

    // Çıktı dizini oluştur
    struct stat st{};
    if (stat(out_dir.c_str(), &st) != 0) mkdir(out_dir.c_str(), 0777);

    // Her assembly → image → sınıflar
    for (size_t ai = 0; ai < count; ++ai) {
        const Il2CppAssembly* asm_ptr = assemblies[ai];
        if (!asm_ptr) continue;

        const Il2CppImage* image = api.assembly_get_image
                                   ? api.assembly_get_image(asm_ptr) : nullptr;
        if (!image) continue;

        const char* img_name = api.image_get_name ? api.image_get_name(image) : nullptr;
        std::string name     = safeStr(img_name, "unknown");
        // .dll uzantısını kaldır
        if (name.size() > 4 && name.substr(name.size() - 4) == ".dll")
            name = name.substr(0, name.size() - 4);

        size_t cls_count = api.image_get_class_count
                           ? api.image_get_class_count(image) : 0;
        LOGI("image[%zu]: %s (%zu sınıf)", ai, name.c_str(), cls_count);

        std::vector<ClassEntry> classes;
        classes.reserve(cls_count);

        for (size_t ci = 0; ci < cls_count; ++ci) {
            Il2CppClass* klass = api.image_get_class
                                 ? api.image_get_class(image, ci) : nullptr;
            if (!klass) continue;

            ClassEntry e;
            e.name        = safeStr(api.class_get_name ? api.class_get_name(klass) : nullptr, "?");
            e.ns          = safeStr(api.class_get_namespace ? api.class_get_namespace(klass) : nullptr);
            e.full_name   = e.ns.empty() ? e.name : (e.ns + "." + e.name);
            e.is_abstract  = api.class_is_abstract  && api.class_is_abstract(klass);
            e.is_interface = api.class_is_interface && api.class_is_interface(klass);
            e.is_enum      = api.class_is_enum      && api.class_is_enum(klass);
            e.is_valuetype = api.class_is_valuetype && api.class_is_valuetype(klass);
            e.is_generic   = api.class_is_generic   && api.class_is_generic(klass);

            Il2CppClass* parent = api.class_get_parent ? api.class_get_parent(klass) : nullptr;
            if (parent) e.parent = safeClassName(parent);

            if (cfg.include_generic || !e.is_generic) {
                e.methods    = extractMethods(klass);
                e.fields     = extractFields(klass);
                e.properties = extractProperties(klass);
            }
            classes.push_back(std::move(e));
        }

        // Dosyalara yaz
        if (cfg.Cpp_Header)   writeCppHeader  (out_dir, name, classes);
        if (cfg.Frida_Script) writeFridaScript (out_dir, name, classes);
    }
    LOGI("runDump: tamamlandı  out_dir=%s", out_dir.c_str());
}

// =============================================================================
//  hackStart  —  KATMAN 4: 30 saniyelik timeout koruması
//
//  libil2cpp.so'yu exponential backoff ile bekler.
//  MAX 30 saniye geçerse döner (sistem kilitlenmez).
//  SADECE hedef uygulama sürecinde çağrılır (Katman 1-2 güvencesi).
// =============================================================================
namespace {

// Bekleme süresi hesabı — exponential backoff, max 2 sn
static int64_t exponentialDelay(int attempt) {
    int64_t ms = 200LL * (1LL << std::min(attempt, 5));
    return std::min(ms, static_cast<int64_t>(2000));
}

// poll() tabanlı stealth bekleme — CPU'yu meşgul etmez
static void stealthWait(int64_t ms) {
    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (efd >= 0) {
        struct pollfd pf = { efd, POLLIN, 0 };
        poll(&pf, 1, static_cast<int>(ms));
        ::close(efd);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// hackStart — Katman 4 timeout ile libil2cpp.so bekler ve dump başlatır
// ─────────────────────────────────────────────────────────────────────────────
static void hackStart(
    const std::string&          game_dir,
    const std::string&          out_dir,
    const config::DumperConfig& cfg
) {
    // ── KATMAN 1 ───────────────────────────────────────────────────────────
    // Bu güvenlik her hackStart() çağrısında yeniden kontrol edilir.
    // Herhangi bir kod yolu Zygote'dan buraya ulaşırsa durur.
    if (isZygoteProcess()) {
        LOGE("[BootGuard] hackStart Zygote'dan çağrıldı! ANINDA ÇIKILIYOR.");
        return;
    }

    LOGI("hackStart başladı  tid=%d  game=%s  out=%s",
         gettid(), game_dir.c_str(), out_dir.c_str());

    // ── KATMAN 4 ───────────────────────────────────────────────────────────
    // 30 saniye timeout — her deneme arasında exponential backoff
    constexpr int   kMaxRetries  = 20;
    constexpr int64_t kMaxTotalMs = 30'000; // 30 saniye
    int64_t elapsed_ms           = 0;

    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (base) {
            LOGI("libil2cpp.so bulundu base=0x%" PRIxPTR " attempt=%d", base, attempt);

            // Protected_Breaker: metadata hook'u kur
            if (cfg.Protected_Breaker && !metadata_hook::s_fired.load())
                metadata_hook::install(base, out_dir);

            runApiInit(base);
            runDump(out_dir, cfg);
            return; // ✅ Başarılı
        }

        // Timeout kontrolü
        auto delay = exponentialDelay(attempt);
        elapsed_ms += delay;
        if (elapsed_ms >= kMaxTotalMs) {
            LOGE("[BootGuard] TIMEOUT: 30 sn içinde libil2cpp.so bulunamadı! Durduruluyor.");
            return; // 🛡️ Bootloop önlendi
        }

        LOGI("libil2cpp.so hazır değil attempt=%d delay=%" PRId64 "ms elapsed=%" PRId64 "ms",
             attempt, delay, elapsed_ms);
        stealthWait(delay);
    }
    LOGE("libil2cpp.so %d denemeden sonra bulunamadı tid=%d", kMaxRetries, gettid());
}

// hackStart'a hazırlık — mimari ve API seviyesini logla
static void hackPrepare(
    const std::string&          game_dir,
    const std::string&          out_dir,
    const config::DumperConfig& cfg
) {
    LOGI("hack_prepare tid=%d", gettid());
    LOGI("api_level=%d", android_get_device_api_level());
#if defined(__arm__)
    LOGI("mode: ARMv7/Thumb-2 (32-bit) [auto-detected]");
#elif defined(__aarch64__)
    LOGI("mode: AArch64 (64-bit) [auto-detected]");
#endif
    hackStart(game_dir, out_dir, cfg);
}

} // anonymous namespace

// =============================================================================
//  EaquelDumperModule  —  ReZygisk Modül Sınıfı
//
//  KATMAN 2: ReZygisk yaşam döngüsü koruması
//  ─────────────────────────────────────────────────────────────────────────
//  onLoad()            → Zygote'da çalışır — HİÇBİR hack kodu yok
//  preAppSpecialize()  → Hedef paket mi? Değilse DLCLOSE_MODULE_LIBRARY
//  postAppSpecialize() → SADECE hedef pakette hack başlatılır
// =============================================================================
class EaquelDumperModule : public rezygisk::ModuleBase {
public:

    // ── onLoad ────────────────────────────────────────────────────────────
    // Zygote içinde çağrılır. Sadece api/env pointer'larını sakla.
    // HAK YOK, TARAMA YOK, THREAD YOK.
    void onLoad(rezygisk::Api* api, JNIEnv* env) override {
        this->api = api;
        this->env = env;
        // Güvenlik: Burada hiçbir şey yapma.
        // Zygote'da çalıştığımız için hack kodu tetiklenmemelidir.
        LOGI("[Module] onLoad: API ve JNIEnv kaydedildi (Zygote güvenli mod)");
    }

    // ── preAppSpecialize ──────────────────────────────────────────────────
    // Uygulama fork edilmeden önce çağrılır.
    // Hedef paketi kontrol et; hedef değilse kütüphaneyi kapat.
    void preAppSpecialize(rezygisk::AppSpecializeArgs* args) override {
        if (!args) {
            // args null ise güvenli tarafta kal
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Denylist'e alınmış process → kapat
        uint32_t flags = api->getFlags();
        if (flags & static_cast<uint32_t>(rezygisk::StateFlag::PROCESS_ON_DENYLIST)) {
            LOGI("[Module] Process denylist'te — modül kapatılıyor");
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Paket adı ve uygulama dizinini al
        const char* pkg = nullptr;
        const char* dir = nullptr;
        if (args->nice_name)    pkg = env->GetStringUTFChars(args->nice_name,    nullptr);
        if (args->app_data_dir) dir = env->GetStringUTFChars(args->app_data_dir, nullptr);

        preSpecialize(pkg ? pkg : "", dir ? dir : "");

        if (pkg) env->ReleaseStringUTFChars(args->nice_name,    pkg);
        if (dir) env->ReleaseStringUTFChars(args->app_data_dir, dir);
    }

    // ── postAppSpecialize ─────────────────────────────────────────────────
    // Uygulama fork edildikten SONRA, SADECE hedef süreçte çağrılır.
    // Artık Zygote değil, uygulama sürecindeyiz — hack başlatmak güvenli.
    void postAppSpecialize(const rezygisk::AppSpecializeArgs*) override {
        if (!enable_hack) return; // Hedef değilse çık

        // ── KATMAN 1 son kontrol ──────────────────────────────────────────
        // postAppSpecialize zaten hedef süreçte çalışır ama bir kez daha
        // kontrol ederek çift güvenlik sağlıyoruz.
        if (isZygoteProcess()) {
            LOGE("[BootGuard] postAppSpecialize Zygote'dan çağrıldı! İptal edildi.");
            return;
        }

        LOGI("[Module] postAppSpecialize: hedef süreçte hack başlatılıyor");

        // SIGSEGV handler kur (sadece hedef süreçte)
        installSigsegvHandler();

        // dlopen hook kur (Protected_Breaker aktifse)
        if (active_cfg.Protected_Breaker) {
            installDlopenHook(out_dir, active_cfg);
        }

        // Hot-reload config watcher başlat
        startConfigWatcher();

        // Erken hook zaten aktifse polling thread gerekmiyor
        if (s_orig_dlopen && s_orig_dlopen != dlopen) {
            LOGI("postAppSpecialize: dlopen early hook aktif, watcher başlatıldı");
            return;
        }

        // Ayrı thread'de hack başlat — Zygisk callback'i bloke etme
        std::string g = game_dir;
        std::string o = out_dir;
        config::DumperConfig c;
        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            c = active_cfg;
        }
        std::thread([g = std::move(g), o = std::move(o), c = std::move(c)]() mutable {
            hackPrepare(g, o, c);
        }).detach();
    }

private:
    rezygisk::Api*       api         = nullptr;
    JNIEnv*              env         = nullptr;
    bool                 enable_hack = false;   // true ise hedef paket
    std::string          game_dir, out_dir;
    config::DumperConfig active_cfg;
    config::ConfigWatcher watcher_;
    std::mutex            cfg_mutex_;
    std::atomic<bool>     dump_running_{false};

    // ─────────────────────────────────────────────────────────────────────
    // triggerReDump — Hot-reload sonrası dump'ı yeniden tetikler
    // ─────────────────────────────────────────────────────────────────────
    void triggerReDump(const config::DumperConfig& new_cfg) {
        bool expected = false;
        if (!dump_running_.compare_exchange_strong(expected, true)) {
            LOGI("hot-reload: dump zaten çalışıyor, atlanıyor");
            return;
        }

        std::string new_out = new_cfg.Output.empty()
                              ? std::string(config::kFallbackOutput)
                              : new_cfg.Output;

        if (new_cfg.Protected_Breaker) {
            metadata_hook::resetForReload(new_out);
        }

        uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (!base) {
            // libil2cpp henüz yüklü değil → hackStart ile bekle
            LOGW("hot-reload: libil2cpp.so henüz yüklü değil, hackStart başlatılıyor");
            std::string g = game_dir;
            config::DumperConfig c = new_cfg;
            std::thread([g = std::move(g), o = new_out, c = std::move(c),
                         this]() mutable {
                hackPrepare(g, o, c);
                dump_running_.store(false, std::memory_order_release);
            }).detach();
            return;
        }

        // Zaten yüklüyse direkt dump
        config::DumperConfig c = new_cfg;
        std::thread([base, o = new_out, c = std::move(c), this]() mutable {
            runApiInit(base);
            runDump(o, c);
            dump_running_.store(false, std::memory_order_release);
        }).detach();
    }

    // ─────────────────────────────────────────────────────────────────────
    // startConfigWatcher — config dosyasındaki değişiklikleri izler
    // ─────────────────────────────────────────────────────────────────────
    void startConfigWatcher() {
        watcher_.start([this](const config::DumperConfig& new_cfg) {
            {
                std::lock_guard<std::mutex> lk(cfg_mutex_);
                active_cfg  = new_cfg;
                s_early_cfg = new_cfg;
            }
            LOGI("hot-reload: config güncellendi Target=%s Cpp=%d Frida=%d Out=%s",
                 new_cfg.Target_Game.c_str(),
                 (int)new_cfg.Cpp_Header,
                 (int)new_cfg.Frida_Script,
                 new_cfg.Output.c_str());

            if (enable_hack) {
                {
                    std::lock_guard<std::mutex> lk(s_early_cfg_mutex);
                    s_early_out_dir = new_cfg.Output.empty()
                                      ? std::string(config::kFallbackOutput)
                                      : new_cfg.Output;
                    s_early_cfg = new_cfg;
                }
                triggerReDump(new_cfg);
            }
        });
    }

    // ─────────────────────────────────────────────────────────────────────
    // preSpecialize — Hedef paket tespiti ve enable_hack bayrağı
    // ─────────────────────────────────────────────────────────────────────
    void preSpecialize(const char* pkg, const char* app_data_dir) {
        active_cfg = config::loadConfig();

        LOGI("preSpecialize: pkg=%s  dir=%s  Target=%s",
             pkg ? pkg : "null",
             app_data_dir ? app_data_dir : "null",
             active_cfg.Target_Game.c_str());

        bool is_target = false;

        if (active_cfg.Target_Game == "!") {
            // Joker: tüm paketler hedef
            is_target = true;
            LOGI("Target_Game=! → joker eşleşme için %s", pkg ? pkg : "?");
        } else if (pkg && !active_cfg.Target_Game.empty()
                   && active_cfg.Target_Game == pkg) {
            // Tam paket adı eşleşmesi
            is_target = true;
        } else if (app_data_dir && !active_cfg.Target_Game.empty()
                   && strstr(app_data_dir, active_cfg.Target_Game.c_str()) != nullptr) {
            // app_data_dir yolunda geçiyor
            is_target = true;
            LOGI("app_data_dir üzerinden hedef algılandı");
        }

        if (!is_target) {
            // Hedef değil → kütüphaneyi kapat, bellek israf etme
            LOGI("Hedef paket değil → modül kapatılıyor");
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        LOGI("HEDEF ALGILANDI: %s  [64bit=%d]",
             active_cfg.Target_Game.c_str(), (int)active_cfg.is_64bit);

        enable_hack = true;
        game_dir    = app_data_dir ? app_data_dir : "";
        out_dir     = active_cfg.Output.empty()
                      ? std::string(config::kFallbackOutput)
                      : active_cfg.Output;

        LOGI("çıktı dizini: %s", out_dir.c_str());
    }
};

// =============================================================================
//  REGISTER_REZYGISK_MODULE — Modülü ReZygisk'e kaydet
//  Bu makro zygisk_module_entry ve rezygisk_module_entry sembollerini üretir.
// =============================================================================
REGISTER_REZYGISK_MODULE(EaquelDumperModule)

// =============================================================================
//  JNI_OnLoad — KATMAN 3: Zygote Güvenliği
//
//  Bu fonksiyon .so dosyası System.loadLibrary() ile yüklendiğinde
//  otomatik tetiklenir. ReZygisk, Zygote'da da bu fonksiyonu çağırır.
//
//  SORUN: Önceki kodda Zygote'dayken hackStart() başlatılıyordu.
//  ÇÖZÜM: isZygoteProcess() → true ise ANINDA JNI_VERSION_1_6 döndür.
//         Zygote olmayan süreçlerde (direkt yükleme, test) normal çalış.
// =============================================================================
#if defined(__arm__) || defined(__aarch64__)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    if (!vm) return JNI_ERR;

    // ── KATMAN 3 ──────────────────────────────────────────────────────────
    // Zygote süreci kontrolü — pozitifse ANINDA döndür
    if (isZygoteProcess()) {
        // Zygote'dayız; hack başlatma, sadece başarılı dön.
        // ReZygisk bu path'ten geçecek; onLoad/preAppSpecialize
        // callback'leri ile doğru süreçte hacklenecek.
        LOGI("[BootGuard] JNI_OnLoad: Zygote süreci — hack devre dışı (güvenli dönüş)");
        return JNI_VERSION_1_6;
    }

    // Buraya ReZygisk callback dışı (direkt dlopen ile yükleme) gelinirse
    // normal çalışma modunu başlat.
    LOGI("[JNI_OnLoad] Hedef süreçte çalışıyoruz, hack başlatılıyor");
    installSigsegvHandler();

    config::DumperConfig cfg = config::loadConfig();
    std::string out = cfg.Output.empty()
                      ? std::string(config::kFallbackOutput)
                      : cfg.Output;
    std::string game;

    // reserved → "game_dir|out_dir" formatında bilgi taşıyabilir
    if (reserved) {
        std::string combined(static_cast<const char*>(reserved));
        auto sep = combined.find('|');
        if (sep != std::string::npos) {
            game = combined.substr(0, sep);
            std::string explicit_out = combined.substr(sep + 1);
            if (!explicit_out.empty()) out = explicit_out;
        } else {
            game = combined;
        }
    }

    if (cfg.Output.empty()) cfg.Output = out;

    std::string o = out;
    std::string g = game;
    config::DumperConfig c = cfg;

    // Thread'de çalıştır — JNI_OnLoad'u bloke etme
    std::thread([g = std::move(g), o = std::move(o), c = std::move(c)]() mutable {
        hackStart(g, o, c); // Katman 4 timeout burada da aktif
    }).detach();

    return JNI_VERSION_1_6;
}
#endif
