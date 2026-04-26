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
#include <unistd.h>
#include <array>
#include <signal.h>
#include <link.h>
#include <mutex>
#include <poll.h>
#include <dirent.h>
#include <span>
#include <algorithm>

[[nodiscard]] static const char* safeStr(const char* s, const char* fallback = "") noexcept {
    return (s && *s) ? s : fallback;
}

static void runApiInit(uintptr_t base);
static void runDump(const std::string& out_dir, const config::DumperConfig& cfg);

static std::string          s_early_out_dir;
static config::DumperConfig s_early_cfg;
static std::atomic<bool>    s_early_hook_fired{false};
static std::mutex           s_early_cfg_mutex;

[[nodiscard]] static bool isZygoteProcess() noexcept {
    char cmdline[256] = {};
    const int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return true;
    const ssize_t n = read(fd, cmdline, sizeof(cmdline) - 1);
    close(fd);
    if (n <= 0) return true;
    const std::string_view sv(cmdline, static_cast<size_t>(n));
    const bool is_zygote = sv.starts_with("zygote");
    if (is_zygote) LOGW("[BootGuard] Zygote process detected cmdline='%s'", cmdline);
    return is_zygote;
}

[[nodiscard]] static int getAndroidApiLevel() noexcept {
    char prop[PROP_VALUE_MAX] = {};
    __system_property_get("ro.build.version.sdk", prop);
    const int api = atoi(prop);
    return (api > 0) ? api : 30;
}

[[nodiscard]] static bool isApiLevelSupported() noexcept {
    const int api = getAndroidApiLevel();
    return api >= config::kAndroidMinApi && api <= config::kAndroidMaxApi;
}

[[nodiscard]] static bool isInstalledAsUserApp(std::string_view pkg) noexcept {
    if (pkg.empty()) return false;
    // Android 11+ şifreli depolama yolları dahil tüm olası lokasyonları kontrol et.
    // /data/data/       → legacy / unencrypted
    // /data/user/0/     → Android 5+ multi-user
    // /data/user_de/0/  → Android 7+ Direct Boot encrypted storage
    // /data/user/999/   → secondary user
    const char* prefixes[] = {
        "/data/data/",
        "/data/user/0/",
        "/data/user_de/0/",
        "/data/user/999/",
    };
    for (const char* prefix : prefixes) {
        const std::string path = std::string(prefix).append(pkg);
        struct stat st{};
        if (stat(path.c_str(), &st) == 0 && st.st_uid >= 10000u) return true;
    }
    return false;
}

[[nodiscard]] static bool shouldIgnoreProcess(std::string_view pkg) noexcept {
    if (pkg.empty()) return true;
    if (process_filter::isSystemProcess(pkg)) return true;
    if (process_filter::isSystemPackage(pkg)) return true;
    return false;
}

namespace hook_engine {

static constexpr size_t kPageSize = 4096u;

[[nodiscard]] static uintptr_t alignDown(uintptr_t a) noexcept {
    return a & ~(kPageSize - 1u);
}

[[nodiscard]] static uintptr_t alignUp(uintptr_t a) noexcept {
    return (a + kPageSize - 1u) & ~(kPageSize - 1u);
}

static bool stealthPatchWindow(uintptr_t addr, size_t len,
                               const std::function<void()>& patch_fn)
{
    void* const  page     = reinterpret_cast<void*>(alignDown(addr));
    const size_t page_len = alignUp(len + (addr - alignDown(addr)));

    if (mprotect(page, page_len, PROT_READ | PROT_WRITE) != 0) {
        LOGE("hook: mprotect RW failed addr=0x%" PRIxPTR " errno=%d", addr, errno);
        return false;
    }
    patch_fn();
    if (mprotect(page, page_len, PROT_READ | PROT_EXEC) != 0) {
        LOGE("hook: mprotect RX failed addr=0x%" PRIxPTR " errno=%d", addr, errno);
        return false;
    }
    return true;
}

static void flushCache(uintptr_t addr, size_t len) noexcept {
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr + len));
}

static void* allocTrampolinePage(uintptr_t near_addr) noexcept {
    const uintptr_t lo = (near_addr > 0x8000000u) ? (near_addr - 0x8000000u) : 0u;
    const uintptr_t hi = near_addr + 0x8000000u;

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
            const uintptr_t try_addr = (prev_end + kPageSize - 1u) & ~(kPageSize - 1u);
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

    if (!result) {
        result = mmap(nullptr, kPageSize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (result == MAP_FAILED) result = nullptr;
    }
    return result;
}

static bool lockTrampolineRX(void* page) noexcept {
    return mprotect(page, kPageSize, PROT_READ | PROT_EXEC) == 0;
}

#if defined(__aarch64__)
static constexpr size_t kHookSize = 16u;

struct Trampoline {
    uint8_t  saved[kHookSize];
    uint32_t ldr_x17;
    uint32_t br_x17;
    uint64_t return_addr;
};

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) {
    const auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[arm64]: trampoline alloc failed"); return false; }

    auto* tramp        = reinterpret_cast<Trampoline*>(tramp_page);
    __builtin_memcpy(tramp->saved, reinterpret_cast<void*>(tgt), kHookSize);
    tramp->ldr_x17     = 0x58000051u;
    tramp->br_x17      = 0xD61F0220u;
    tramp->return_addr = static_cast<uint64_t>(tgt + kHookSize);
    flushCache(reinterpret_cast<uintptr_t>(tramp), sizeof(Trampoline));

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[arm64]: trampoline lock RX failed"); return false;
    }

    const bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        auto* const dst       = reinterpret_cast<uint8_t*>(tgt);
        const uint32_t ldr    = 0x58000051u;
        const uint32_t br     = 0xD61F0220u;
        __builtin_memcpy(dst,     &ldr, 4);
        __builtin_memcpy(dst + 4, &br,  4);
        const uint64_t hook_addr = reinterpret_cast<uint64_t>(hook);
        __builtin_memcpy(dst + 8, &hook_addr, 8);
        flushCache(tgt, kHookSize);
    });

    if (!ok) return false;
    if (orig_out) *orig_out = reinterpret_cast<void*>(tramp);
    LOGI("hook[arm64]: target=%p hook=%p tramp=%p", target, hook, tramp);
    return true;
}

#elif defined(__arm__)
static constexpr size_t kHookSize = 8u;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) {
    const auto tgt_thumb = reinterpret_cast<uintptr_t>(target);
    const auto tgt       = tgt_thumb & ~1u;
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[arm32]: trampoline alloc failed"); return false; }

    __builtin_memcpy(tramp_page, reinterpret_cast<void*>(tgt), kHookSize);
    const uint8_t jback[] = { 0xDF, 0xF8, 0x00, 0xF0 };
    __builtin_memcpy(tramp_page + kHookSize, jback, 4);
    const uint32_t ret_addr = static_cast<uint32_t>(tgt + kHookSize) | 1u;
    __builtin_memcpy(tramp_page + kHookSize + 4, &ret_addr, 4);
    flushCache(reinterpret_cast<uintptr_t>(tramp_page), kHookSize + 8u);

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[arm32]: trampoline lock RX failed"); return false;
    }

    const bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        const uint8_t ldr_pc[] = { 0xDF, 0xF8, 0x00, 0xF0 };
        __builtin_memcpy(reinterpret_cast<void*>(tgt), ldr_pc, 4);
        const uint32_t hook_addr = reinterpret_cast<uint32_t>(hook) | 1u;
        __builtin_memcpy(reinterpret_cast<void*>(tgt + 4), &hook_addr, 4);
        flushCache(tgt, kHookSize);
    });

    if (!ok) return false;
    if (orig_out) *orig_out = reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(tramp_page) | 1u);
    LOGI("hook[arm32]: target=%p hook=%p tramp=%p", target, hook, tramp_page);
    return true;
}

#elif defined(__x86_64__) || defined(__i386__)
static constexpr size_t kHookSize = 14u;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) {
    const auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[x86]: trampoline alloc failed"); return false; }

    __builtin_memcpy(tramp_page, reinterpret_cast<void*>(tgt), kHookSize);
    flushCache(reinterpret_cast<uintptr_t>(tramp_page), kHookSize);

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[x86]: trampoline lock RX failed"); return false;
    }

    const bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        auto* const dst = reinterpret_cast<uint8_t*>(tgt);
#if defined(__x86_64__)
        dst[0] = 0xFF; dst[1] = 0x25;
        const uint32_t rel = 0u;
        __builtin_memcpy(dst + 2, &rel, 4);
        const uint64_t hook_addr = reinterpret_cast<uint64_t>(hook);
        __builtin_memcpy(dst + 6, &hook_addr, 8);
#else
        dst[0] = 0xE9;
        const uint32_t rel = static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(hook) - tgt - 5u);
        __builtin_memcpy(dst + 1, &rel, 4);
#endif
        flushCache(tgt, kHookSize);
    });

    if (!ok) return false;
    if (orig_out) *orig_out = reinterpret_cast<void*>(tramp_page);
    LOGI("hook[x86]: target=%p hook=%p tramp=%p", target, hook, tramp_page);
    return true;
}

#else
[[nodiscard]] bool installInlineHook(void*, void*, void**) noexcept { return false; }
#endif

[[nodiscard]] bool patchGotSlot(void* target_fn, void* hook_fn, void** orig_out) {
    Dl_info di{};
    if (!dladdr(reinterpret_cast<void*>(&patchGotSlot), &di) || !di.dli_fbase) {
        LOGE("GOT patch: dladdr failed"); return false;
    }
    const auto self_base = reinterpret_cast<uintptr_t>(di.dli_fbase);

    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return false;
    char line[512];

    while (fgets(line, sizeof(line), maps)) {
        uintptr_t seg_s = 0, seg_e = 0;
        char perms[8] = {};
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %7s", &seg_s, &seg_e, perms) < 3) continue;
        if (perms[1] != 'w') continue;
        if (seg_s < self_base || seg_s > self_base + 0x8000000u) continue;

        for (uintptr_t addr = seg_s; addr + sizeof(void*) <= seg_e; addr += sizeof(void*)) {
            void** slot = reinterpret_cast<void**>(addr);
            if (*slot != target_fn) continue;
            const uintptr_t page = alignDown(addr);
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

static void installSigsegvHandler() noexcept {
    struct sigaction sa{};
    sa.sa_sigaction = scanner::sigsegv_handler;
    sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
}

namespace metadata_hook {

using MetadataLoadFn = void* (*)(const char* path, size_t* out_size);

static MetadataLoadFn    s_orig_load = nullptr;
static std::atomic<bool> s_fired{false};
static std::string       s_dump_dir;
static std::mutex        s_mutex;

static void persistDecryptedMetadata(const void* data, size_t size, const char* src_path) {
    if (!data || size < 4u) return;

    const auto* buf   = reinterpret_cast<const uint8_t*>(data);
    const auto  state = entropy::analyzeBuffer(buf, size);
    std::vector<uint8_t> plaintext;

    if (state == entropy::MetadataState::Encrypted) {
        LOGI("[MetaHook] Buffer encrypted -- XOR key scan starting");
        const auto key = entropy::discoverXorKey(buf, size);
        if (key.found && key.score >= 2) {
            plaintext = entropy::decryptBuffer(buf, size, key);
            LOGI("[MetaHook] Decrypt done key_len=%zu score=%d", key.key_len, key.score);
        } else {
            LOGW("[MetaHook] XOR key unverified -- writing raw buffer");
            plaintext.assign(buf, buf + size);
        }
    } else {
        plaintext.assign(buf, buf + size);
    }

    uint32_t out_magic = 0;
    if (plaintext.size() >= 4u) __builtin_memcpy(&out_magic, plaintext.data(), 4);
    if (out_magic != entropy::kIl2CppMetadataMagic)
        LOGW("[MetaHook] Output magic 0x%08X -- may still be encrypted", out_magic);

    std::lock_guard<std::mutex> lk(s_mutex);
    struct stat st{};
    if (stat(s_dump_dir.c_str(), &st) != 0) mkdir(s_dump_dir.c_str(), 0777);

    const std::string out_path = s_dump_dir + "/global-metadata.dat";
    const int fd = open(out_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) {
        LOGE("[MetaHook] open(%s) failed errno=%d", out_path.c_str(), errno);
        return;
    }

    const uint8_t* ptr  = plaintext.data();
    size_t         left = plaintext.size();
    while (left > 0) {
        const ssize_t w = write(fd, ptr, left);
        if (w <= 0) break;
        ptr  += static_cast<size_t>(w);
        left -= static_cast<size_t>(w);
    }
    fdatasync(fd);
    close(fd);
    LOGI("[MetaHook] metadata written (%zu bytes) %s -> %s",
         plaintext.size(), src_path ? src_path : "?", out_path.c_str());
}

static void* hooked_MetadataLoad(const char* path, size_t* out_size) {
    void* result = s_orig_load(path, out_size);
    bool  expected = false;
    if (result && out_size && s_fired.compare_exchange_strong(expected, true)) {
        LOGI("[MetaHook] triggered path=%s size=%zu", path ? path : "?", *out_size);
        std::thread([data  = result,
                     size  = *out_size,
                     spath = std::string(path ? path : "")]() {
            persistDecryptedMetadata(data, size, spath.c_str());
        }).detach();
    }
    return result;
}

static bool install(uintptr_t lib_base, const std::string& dump_dir) {
    {
        std::lock_guard<std::mutex> lk(s_mutex);
        s_dump_dir = dump_dir;
    }
    const auto syms        = scanner::scanAllSymbols(lib_base);
    uintptr_t  loader_addr = 0;

#if defined(__aarch64__)
    loader_addr = syms.metadata_cache_initialize
                  ? syms.metadata_cache_initialize
                  : syms.metadata_loader;
#elif defined(__arm__)
    loader_addr = syms.arm32_metadata_cache_init
                  ? syms.arm32_metadata_cache_init
                  : syms.arm32_metadata_loader;
#elif defined(__x86_64__) || defined(__i386__)
    loader_addr = syms.metadata_cache_initialize
                  ? syms.metadata_cache_initialize
                  : syms.metadata_loader;
#endif

    if (!loader_addr) { LOGW("[MetaHook] loader address not found"); return false; }

    const bool ok = hook_engine::installInlineHook(
        reinterpret_cast<void*>(loader_addr),
        reinterpret_cast<void*>(&hooked_MetadataLoad),
        reinterpret_cast<void**>(&s_orig_load)
    );

    if (ok) LOGI("[MetaHook] inline hook installed 0x%" PRIxPTR, loader_addr);
    else    LOGE("[MetaHook] inline hook failed");
    return ok;
}

static void resetForReload(const std::string& new_dump_dir) {
    std::lock_guard<std::mutex> lk(s_mutex);
    s_dump_dir = new_dump_dir;
    s_fired.store(false, std::memory_order_release);
    LOGI("[MetaHook] hot-reload reset dump_dir=%s", new_dump_dir.c_str());
}

} // namespace metadata_hook

static void* (*s_orig_dlopen)(const char*, int) = nullptr;

static void* hooked_dlopen(const char* path, int flags) {
    void* handle = s_orig_dlopen(path, flags);
    if (handle && path && strstr(path, "libil2cpp.so")) {
        bool expected = false;
        if (s_early_hook_fired.compare_exchange_strong(expected, true)) {
            LOGI("EARLY HOOK: libil2cpp.so loaded path=%s", path);

            std::string out;
            config::DumperConfig cfg;
            {
                std::lock_guard<std::mutex> lk(s_early_cfg_mutex);
                out = s_early_out_dir;
                cfg = s_early_cfg;
            }

            std::thread([out = std::move(out), cfg = std::move(cfg), handle]() mutable {
                int efd = eventfd(0u, EFD_CLOEXEC | EFD_NONBLOCK);
                if (efd >= 0) {
                    struct pollfd pf = { efd, POLLIN, 0 };
                    poll(&pf, 1, 30);
                    ::close(efd);
                }

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

static bool installDlopenHook(const std::string& out_dir,
                               const config::DumperConfig& cfg)
{
    {
        std::lock_guard<std::mutex> lk(s_early_cfg_mutex);
        s_early_out_dir = out_dir;
        s_early_cfg     = cfg;
    }
    s_early_hook_fired.store(false, std::memory_order_release);

    if (hook_engine::installInlineHook(
            reinterpret_cast<void*>(dlopen),
            reinterpret_cast<void*>(hooked_dlopen),
            reinterpret_cast<void**>(&s_orig_dlopen)))
    {
        LOGI("dlopen hook installed (inline hook)");
        return true;
    }

    if (hook_engine::patchGotSlot(
            reinterpret_cast<void*>(dlopen),
            reinterpret_cast<void*>(hooked_dlopen),
            reinterpret_cast<void**>(&s_orig_dlopen)))
    {
        LOGI("dlopen hook installed (GOT patch)");
        s_orig_dlopen = dlopen;
        return true;
    }

    LOGW("dlopen hook failed -- polling fallback mode");
    return false;
}

namespace il2cpp_api {

struct FunctionTable {
    bool                   (*init)(const char*, int)                               = nullptr;
    Il2CppDomain*          (*domain_get)()                                         = nullptr;
    const Il2CppAssembly** (*domain_get_assemblies)(const Il2CppDomain*, size_t*)  = nullptr;
    const Il2CppImage*     (*assembly_get_image)(const Il2CppAssembly*)            = nullptr;
    const char*            (*image_get_name)(const Il2CppImage*)                   = nullptr;
    size_t                 (*image_get_class_count)(const Il2CppImage*)            = nullptr;
    Il2CppClass*           (*image_get_class)(const Il2CppImage*, size_t)          = nullptr;
    const Il2CppType*      (*class_get_type)(Il2CppClass*)                         = nullptr;
    Il2CppClass*           (*class_from_type)(const Il2CppType*)                   = nullptr;
    const char*            (*class_get_name)(Il2CppClass*)                         = nullptr;
    const char*            (*class_get_namespace)(Il2CppClass*)                    = nullptr;
    uint32_t               (*class_get_flags)(Il2CppClass*)                        = nullptr;
    bool                   (*class_is_valuetype)(Il2CppClass*)                     = nullptr;
    bool                   (*class_is_enum)(Il2CppClass*)                          = nullptr;
    bool                   (*class_is_abstract)(Il2CppClass*)                      = nullptr;
    bool                   (*class_is_interface)(Il2CppClass*)                     = nullptr;
    bool                   (*class_is_generic)(Il2CppClass*)                       = nullptr;
    bool                   (*class_is_inflated)(Il2CppClass*)                      = nullptr;
    Il2CppClass*           (*class_get_parent)(Il2CppClass*)                       = nullptr;
    Il2CppClass*           (*class_get_interfaces)(Il2CppClass*, void**)           = nullptr;
    Il2CppClass*           (*class_get_nested_types)(Il2CppClass*, void**)         = nullptr;
    FieldInfo*             (*class_get_fields)(Il2CppClass*, void**)               = nullptr;
    PropertyInfo*          (*class_get_properties)(Il2CppClass*, void**)           = nullptr;
    const MethodInfo*      (*class_get_methods)(Il2CppClass*, void**)              = nullptr;
    const MethodInfo*      (*class_get_method_from_name)(Il2CppClass*, const char*, int) = nullptr;
    Il2CppClass*           (*class_from_name)(const Il2CppImage*, const char*, const char*) = nullptr;
    Il2CppClass*           (*class_from_system_type)(Il2CppReflectionType*)        = nullptr;
    const Il2CppImage*     (*get_corlib)()                                         = nullptr;
    uint32_t               (*field_get_flags)(FieldInfo*)                          = nullptr;
    const char*            (*field_get_name)(FieldInfo*)                           = nullptr;
    size_t                 (*field_get_offset)(FieldInfo*)                         = nullptr;
    const Il2CppType*      (*field_get_type)(FieldInfo*)                           = nullptr;
    void                   (*field_static_get_value)(FieldInfo*, void*)            = nullptr;
    uint32_t               (*property_get_flags)(PropertyInfo*)                    = nullptr;
    const MethodInfo*      (*property_get_get_method)(PropertyInfo*)               = nullptr;
    const MethodInfo*      (*property_get_set_method)(PropertyInfo*)               = nullptr;
    const char*            (*property_get_name)(PropertyInfo*)                     = nullptr;
    const Il2CppType*      (*method_get_return_type)(const MethodInfo*)            = nullptr;
    const char*            (*method_get_name)(const MethodInfo*)                   = nullptr;
    uint32_t               (*method_get_param_count)(const MethodInfo*)            = nullptr;
    const Il2CppType*      (*method_get_param)(const MethodInfo*, uint32_t)        = nullptr;
    uint32_t               (*method_get_flags)(const MethodInfo*, uint32_t*)       = nullptr;
    const char*            (*method_get_param_name)(const MethodInfo*, uint32_t)   = nullptr;
    bool                   (*type_is_byref)(const Il2CppType*)                     = nullptr;
    Il2CppThread*          (*thread_attach)(Il2CppDomain*)                         = nullptr;
    bool                   (*is_vm_thread)(Il2CppThread*)                          = nullptr;
    Il2CppString*          (*string_new)(const char*)                              = nullptr;
};

static FunctionTable g_api;
static uint64_t      g_il2cpp_base = 0;

template<typename T>
static void resolveSymbol(void* handle, const char* name, T& out) noexcept {
    if (!handle || !name) return;
    void* sym = dlsym(handle, name);
    if (sym) out = reinterpret_cast<T>(sym);
    else     LOGW("symbol not resolved: %s", name);
}

template<typename T>
static void applyFallback(uintptr_t addr, T& out) noexcept {
    if (!out && addr && scanner::isReadableAddress(addr)) {
        out = reinterpret_cast<T>(addr);
        LOGI("scanner fallback: 0x%" PRIxPTR, addr);
    }
}

static void populateFunctionTable(uintptr_t base) {
    void* handle = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
    if (!handle) LOGW("dlopen RTLD_NOLOAD failed -- pattern scan mode");

    if (handle) {
        auto r = [&](const char* name, auto& out) { resolveSymbol(handle, name, out); };

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

    const bool critical_missing =
        !g_api.domain_get || !g_api.domain_get_assemblies ||
        !g_api.image_get_class || !g_api.thread_attach || !g_api.is_vm_thread;

    if (critical_missing) {
        LOGW("Critical symbols missing -- full adaptive pattern scan starting");
        const auto s = scanner::scanAllSymbols(base);
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
        if (!g_api.domain_get)            LOGE("FATAL: domain_get unresolved");
        if (!g_api.domain_get_assemblies) LOGE("FATAL: domain_get_assemblies unresolved");
        if (!g_api.image_get_class)       LOGW("image_get_class missing -- reflection fallback");
    }
}

} // namespace il2cpp_api

static std::string buildMethodModifier(uint32_t flags) {
    std::string out;
    out.reserve(32);
    switch (flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK) {
        case METHOD_ATTRIBUTE_PRIVATE:       out += "private ";            break;
        case METHOD_ATTRIBUTE_PUBLIC:        out += "public ";             break;
        case METHOD_ATTRIBUTE_FAMILY:        out += "protected ";          break;
        case METHOD_ATTRIBUTE_ASSEM:
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM: out += "internal ";           break;
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM:  out += "protected internal "; break;
        default: break;
    }
    if (flags & METHOD_ATTRIBUTE_STATIC)   out += "static ";
    if (flags & METHOD_ATTRIBUTE_ABSTRACT) {
        out += "abstract ";
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            out += "override ";
    } else if (flags & METHOD_ATTRIBUTE_FINAL) {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            out += "sealed override ";
    } else if (flags & METHOD_ATTRIBUTE_VIRTUAL) {
        out += ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT)
               ? "virtual " : "override ";
    }
    if (flags & METHOD_ATTRIBUTE_PINVOKE_IMPL) out += "extern ";
    return out;
}

static std::string safeClassName(Il2CppClass* klass) {
    if (!klass) return "object";
    auto& api        = il2cpp_api::g_api;
    const auto* name = api.class_get_name ? api.class_get_name(klass) : nullptr;
    return std::string(safeStr(name, "object"));
}

struct MethodEntry {
    std::string name, return_type, modifier;
    uint64_t    rva = 0, va = 0;
    std::vector<std::pair<std::string, std::string>> params;
};

struct FieldEntry {
    std::string name, type_name, modifier;
    uint64_t    offset    = 0;
    bool        is_static = false;
};

struct PropertyEntry {
    std::string name, type_name, modifier;
    bool        has_getter = false, has_setter = false;
};

struct ClassEntry {
    std::string name, ns, parent, full_name;
    bool        is_abstract   = false;
    bool        is_interface  = false;
    bool        is_enum       = false;
    bool        is_valuetype  = false;
    bool        is_generic    = false;
    std::vector<MethodEntry>   methods;
    std::vector<FieldEntry>    fields;
    std::vector<PropertyEntry> properties;
    std::vector<std::string>   interfaces;
};

static std::string resolveTypeName(const Il2CppType* t) {
    if (!t) return "void";
    auto& api = il2cpp_api::g_api;

    const bool is_byref    = api.type_is_byref && api.type_is_byref(t);
    Il2CppClass* klass = api.class_from_type ? api.class_from_type(t) : nullptr;
    std::string  base  = safeClassName(klass);

    if (klass && api.class_is_generic && api.class_is_generic(klass)) {
        base += "<>";
    } else if (klass && api.class_is_inflated && api.class_is_inflated(klass)) {
        base += "<T>";
    }

    if (t->type == IL2CPP_TYPE_SZARRAY || t->type == IL2CPP_TYPE_ARRAY)
        base += "[]";

    if (is_byref) base = "ref " + base;
    return base;
}

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
        const uint32_t flags = api.method_get_flags ? api.method_get_flags(method, &impl_flags) : 0;
        e.modifier = buildMethodModifier(flags);

        const Il2CppType* rt = api.method_get_return_type
                               ? api.method_get_return_type(method) : nullptr;
        e.return_type = resolveTypeName(rt);

        const uint32_t pcount = api.method_get_param_count
                                ? api.method_get_param_count(method) : 0;
        for (uint32_t i = 0; i < pcount; ++i) {
            const Il2CppType* pt = api.method_get_param
                                   ? api.method_get_param(method, i) : nullptr;
            const char* pn       = api.method_get_param_name
                                   ? api.method_get_param_name(method, i) : nullptr;
            e.params.emplace_back(resolveTypeName(pt), safeStr(pn, "arg"));
        }

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

static std::vector<FieldEntry> extractFields(Il2CppClass* klass) {
    std::vector<FieldEntry> result;
    auto& api = il2cpp_api::g_api;
    if (!api.class_get_fields) return result;

    void*      iter  = nullptr;
    FieldInfo* field = nullptr;
    while ((field = api.class_get_fields(klass, &iter)) != nullptr) {
        FieldEntry e;
        e.name      = safeStr(api.field_get_name ? api.field_get_name(field) : nullptr, "?");
        const Il2CppType* ft = api.field_get_type ? api.field_get_type(field) : nullptr;
        e.type_name = resolveTypeName(ft);
        e.offset    = api.field_get_offset
                      ? static_cast<uint64_t>(api.field_get_offset(field)) : 0u;
        const uint32_t fg = api.field_get_flags ? api.field_get_flags(field) : 0;
        e.is_static = (fg & FIELD_ATTRIBUTE_STATIC) != 0;
        e.modifier  = e.is_static ? "static " : "";
        result.push_back(std::move(e));
    }
    return result;
}

static std::vector<PropertyEntry> extractProperties(Il2CppClass* klass) {
    std::vector<PropertyEntry> result;
    auto& api = il2cpp_api::g_api;
    if (!api.class_get_properties) return result;

    void*         iter = nullptr;
    PropertyInfo* prop = nullptr;
    while ((prop = api.class_get_properties(klass, &iter)) != nullptr) {
        PropertyEntry e;
        e.name       = safeStr(api.property_get_name ? api.property_get_name(prop) : nullptr, "?");
        e.has_getter = api.property_get_get_method && (api.property_get_get_method(prop) != nullptr);
        e.has_setter = api.property_get_set_method && (api.property_get_set_method(prop) != nullptr);
        const MethodInfo* getter = e.has_getter ? api.property_get_get_method(prop) : nullptr;
        if (getter) {
            const Il2CppType* pt = api.method_get_return_type
                                   ? api.method_get_return_type(getter) : nullptr;
            e.type_name = resolveTypeName(pt);
        }
        result.push_back(std::move(e));
    }
    return result;
}

static void ensureDirectory(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) mkdir(path.c_str(), 0777);
}

static void writeCppHeader(const std::string& out_dir,
                           const std::string& image_name,
                           const std::vector<ClassEntry>& classes)
{
    ensureDirectory(out_dir);

    const std::string path = out_dir + "/" + image_name + ".h";
    async_io::AsyncWriter w;
    if (!w.open(path)) { LOGE("header write: %s failed", path.c_str()); return; }

    w.write("#pragma once\n#include <cstdint>\n\n");

    for (const auto& cls : classes) {
        std::string block;
        block.reserve(4096);

        if (!cls.ns.empty()) { block += "namespace "; block += cls.ns; block += " {\n"; }

        if (cls.is_interface)      { block += "struct /* interface */ "; block += cls.name; }
        else if (cls.is_enum)      { block += "enum class "; block += cls.name; }
        else                       { block += "struct "; block += cls.name; }

        if (!cls.parent.empty() && cls.parent != "object") {
            block += " : public "; block += cls.parent;
        }
        block += " {\n";

        for (const auto& fld : cls.fields) {
            char hex[32];
            snprintf(hex, sizeof(hex), "%" PRIx64, fld.offset);
            block += "    "; block += fld.modifier; block += fld.type_name;
            block += " "; block += fld.name; block += "; // 0x"; block += hex; block += "\n";
        }

        for (const auto& m : cls.methods) {
            char rva[32];
            snprintf(rva, sizeof(rva), "%" PRIx64, m.rva);
            block += "    "; block += m.modifier; block += m.return_type;
            block += " "; block += m.name; block += "(";
            for (size_t i = 0; i < m.params.size(); ++i) {
                if (i) block += ", ";
                block += m.params[i].first; block += " "; block += m.params[i].second;
            }
            block += "); // RVA: 0x"; block += rva; block += "\n";
        }

        block += "};\n";
        if (!cls.ns.empty()) { block += "} // namespace "; block += cls.ns; block += "\n"; }
        block += "\n";
        w.write(block);
    }

    w.close();
    LOGI("C++ header written: %s (%zu classes)", path.c_str(), classes.size());
}

static void writeFridaScript(const std::string& out_dir,
                             const std::string& image_name,
                             const std::vector<ClassEntry>& classes)
{
    ensureDirectory(out_dir);

    const std::string path = out_dir + "/" + image_name + ".js";
    async_io::AsyncWriter w;
    if (!w.open(path)) { LOGE("frida script write: %s failed", path.c_str()); return; }

    w.write("var il2cpp_base = Module.findBaseAddress('libil2cpp.so');\n");
    w.write("if (!il2cpp_base) throw new Error('libil2cpp.so not found');\n\n");

    for (const auto& cls : classes) {
        for (const auto& m : cls.methods) {
            if (!m.rva) continue;
            char rva[32];
            snprintf(rva, sizeof(rva), "%" PRIx64, m.rva);
            std::string entry;
            entry.reserve(256);
            entry += "var addr_"; entry += cls.name; entry += "_"; entry += m.name;
            entry += " = il2cpp_base.add(0x"; entry += rva; entry += ");\n";
            entry += "Interceptor.attach(addr_"; entry += cls.name; entry += "_"; entry += m.name;
            entry += ", {\n    onEnter: function(args) {},\n    onLeave: function(retval) {}\n});\n\n";
            w.write(entry);
        }
    }

    w.close();
    LOGI("Frida script written: %s (%zu classes)", path.c_str(), classes.size());
}

static void runApiInit(uintptr_t base) {
    il2cpp_api::g_il2cpp_base = static_cast<uint64_t>(base);
    il2cpp_api::populateFunctionTable(base);
}

static void runDump(const std::string& out_dir, const config::DumperConfig& cfg) {
    auto& api = il2cpp_api::g_api;
    if (!api.domain_get) {
        LOGE("runDump: domain_get null -- dump cancelled");
        return;
    }

    Il2CppDomain* domain = api.domain_get();
    if (!domain) { LOGE("runDump: domain null"); return; }

    if (api.thread_attach) api.thread_attach(domain);

    size_t count = 0;
    const Il2CppAssembly** assemblies = api.domain_get_assemblies
                                        ? api.domain_get_assemblies(domain, &count)
                                        : nullptr;
    if (!assemblies || !count) { LOGE("runDump: assembly list empty"); return; }

    LOGI("runDump: %zu assemblies found", count);
    ensureDirectory(out_dir);

    for (size_t ai = 0; ai < count; ++ai) {
        const Il2CppAssembly* asm_ptr = assemblies[ai];
        if (!asm_ptr) continue;

        const Il2CppImage* image = api.assembly_get_image
                                   ? api.assembly_get_image(asm_ptr) : nullptr;
        if (!image) continue;

        const char* img_name = api.image_get_name ? api.image_get_name(image) : nullptr;
        std::string name     = safeStr(img_name, "unknown");
        if (name.size() > 4u && name.substr(name.size() - 4u) == ".dll")
            name = name.substr(0u, name.size() - 4u);

        const size_t cls_count = api.image_get_class_count
                                 ? api.image_get_class_count(image) : 0u;
        LOGI("image[%zu]: %s (%zu classes)", ai, name.c_str(), cls_count);

        std::vector<ClassEntry> classes;
        classes.reserve(cls_count);

        for (size_t ci = 0; ci < cls_count; ++ci) {
            Il2CppClass* klass = api.image_get_class
                                 ? api.image_get_class(image, ci) : nullptr;
            if (!klass) continue;

            ClassEntry e;
            e.name        = safeStr(api.class_get_name     ? api.class_get_name(klass)     : nullptr, "?");
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

        if (cfg.Cpp_Header)   writeCppHeader  (out_dir, name, classes);
        if (cfg.Frida_Script) writeFridaScript (out_dir, name, classes);
    }
    LOGI("runDump: complete  out_dir=%s", out_dir.c_str());
}

namespace {

[[nodiscard]] static int64_t exponentialDelay(int attempt) noexcept {
    const int64_t ms = 200LL * (1LL << std::min(attempt, 5));
    return std::min(ms, static_cast<int64_t>(2000));
}

static void stealthWait(int64_t ms) noexcept {
    int efd = eventfd(0u, EFD_CLOEXEC | EFD_NONBLOCK);
    if (efd >= 0) {
        struct pollfd pf = { efd, POLLIN, 0 };
        poll(&pf, 1, static_cast<int>(ms));
        ::close(efd);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

static void hackStart(
    const std::string&          game_dir,
    const std::string&          out_dir,
    const config::DumperConfig& cfg)
{
    if (isZygoteProcess()) {
        LOGE("[BootGuard] hackStart called from Zygote -- aborting");
        return;
    }
    if (!isApiLevelSupported()) {
        LOGE("[ApiGuard] Android API %d not in range [%d-%d] -- aborting",
             getAndroidApiLevel(), config::kAndroidMinApi, config::kAndroidMaxApi);
        return;
    }

    LOGI("hackStart tid=%d  game=%s  out=%s  api=%d",
         gettid(), game_dir.c_str(), out_dir.c_str(), getAndroidApiLevel());

    // FIX: Bazı oyunlar (özellikle büyük Unity/IL2CPP oyunları) libil2cpp.so'yu
    // geç yükler. 30 saniye ve 20 deneme yetmeyebilir.
    // Artık 60 saniye / 30 deneme yapıyoruz. Ayrıca her beklemeyi logla.
    constexpr int     kMaxRetries = 30;
    constexpr int64_t kMaxTotalMs = 60'000LL;
    int64_t           elapsed_ms  = 0;

    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        const uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (base) {
            LOGI("libil2cpp.so found base=0x%" PRIxPTR " attempt=%d elapsed=%" PRId64 "ms",
                 base, attempt, elapsed_ms);
            if (cfg.Protected_Breaker && !metadata_hook::s_fired.load())
                metadata_hook::install(base, out_dir);
            runApiInit(base);
            runDump(out_dir, cfg);
            return;
        }

        const auto delay = exponentialDelay(attempt);
        elapsed_ms += delay;
        if (elapsed_ms >= kMaxTotalMs) {
            LOGE("[BootGuard] TIMEOUT: libil2cpp.so not found in %" PRId64 "ms -- stopping", kMaxTotalMs);
            return;
        }
        // FIX: Her beklemeyi logla (sadece ilk logcat'e değil, süreç takibi için)
        LOGI("libil2cpp.so not ready attempt=%d/%d delay=%" PRId64 "ms total_elapsed=%" PRId64 "ms",
             attempt + 1, kMaxRetries, delay, elapsed_ms);
        stealthWait(delay);
    }
    LOGE("libil2cpp.so not found after %d attempts tid=%d", kMaxRetries, gettid());
}

static void hackPrepare(
    const std::string&          game_dir,
    const std::string&          out_dir,
    const config::DumperConfig& cfg)
{
    LOGI("hack_prepare tid=%d api=%d", gettid(), getAndroidApiLevel());
#if defined(__arm__)
    LOGI("mode: ARMv7/Thumb-2 (32-bit mobile)");
#elif defined(__aarch64__)
    LOGI("mode: AArch64 (64-bit mobile)");
#elif defined(__x86_64__)
    LOGI("mode: x86_64 (64-bit emulator)");
#elif defined(__i386__)
    LOGI("mode: x86 (32-bit emulator)");
#endif
    hackStart(game_dir, out_dir, cfg);
}

} // anonymous namespace

class EaquelDumperModule : public rezygisk::ModuleBase {
public:

    void onLoad(rezygisk::Api* api_in, JNIEnv* env_in) override {
        // FIX: Parametre ismi sınıf üyesiyle çakışıyordu (shadow bug).
        // Eski kod: api_ = api_; → kendini kendine atıyor, this->api NULL kalıyordu.
        if (!api_in) {
            LOGE("[Module] onLoad: api pointer NULL -- module cannot function");
            return;
        }
        this->api = api_in;
        this->env = env_in;
        LOGI("[Module] onLoad: registered -- Zygote safe, waiting for preAppSpecialize");
    }

    void preAppSpecialize(rezygisk::AppSpecializeArgs* args) override {
        // FIX: api pointer NULL ise hiçbir şey yapamayız, erken çık
        if (!api) {
            LOGE("[Module] preAppSpecialize: api is NULL (onLoad bug not fixed?) -- skip");
            return;
        }

        if (!args) {
            LOGI("[Module] preAppSpecialize: args NULL -- unloading");
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const char* raw_nice  = nullptr;
        const char* raw_dir   = nullptr;
        const char* raw_iset  = nullptr;
        if (args->nice_name)       raw_nice = env->GetStringUTFChars(args->nice_name,       nullptr);
        if (args->app_data_dir)    raw_dir  = env->GetStringUTFChars(args->app_data_dir,    nullptr);
        if (args->instruction_set) raw_iset = env->GetStringUTFChars(args->instruction_set, nullptr);

        const std::string nice_str = raw_nice ? raw_nice : "";
        const std::string dir_str  = raw_dir  ? raw_dir  : "";
        const std::string iset_str = raw_iset ? raw_iset : "";  // "arm", "arm64", "x86" vb.

        // ══════════════════════════════════════════════════════════════════════
        // pkg_str extraction — 4 katmanlı güvenli fallback zinciri
        //
        // Geçmiş hatalar:
        //  • ReZygisk.hpp'de `rlimits` field eksikti → struct layout kayıyordu
        //    → app_data_dir yerine instruction_set okunuyordu → dir='arm'
        //  • Trailing slash → rfind son char → substr boş
        //  • Invisible char → string::== başarısız
        //  • Sistem prosesleri için app_data_dir zaten NULL gelir (normal davranış)
        //
        // Çözümler uygulandı (ReZygisk.hpp ABI fix + aşağıdaki 4 katman):
        // ══════════════════════════════════════════════════════════════════════

        auto sanitizePkg = [](std::string s) -> std::string {
            // Görünmez karakter / null byte / control char temizle
            s.erase(std::remove_if(s.begin(), s.end(),
                        [](unsigned char c) { return c < 0x20u || c > 0x7Eu; }),
                    s.end());
            const size_t f = s.find_first_not_of(" \t\r\n");
            if (f == std::string::npos) return {};
            return s.substr(f, s.find_last_not_of(" \t\r\n") - f + 1u);
        };

        auto isValidPkg = [](const std::string& s) -> bool {
            if (s.size() < 3u) return false;
            if (s.find('.') == std::string::npos) return false;
            if (s.find(':') != std::string::npos) return false;  // selinux etiketi
            return std::all_of(s.begin(), s.end(), [](unsigned char c) {
                return std::isalnum(c) || c == '.' || c == '_' || c == '-';
            });
        };

        auto extractPkgFromDir = [&](const std::string& dir_in) -> std::string {
            if (dir_in.empty()) return {};
            std::string dir = dir_in;
            while (!dir.empty() && dir.back() == '/') dir.pop_back();
            const size_t slash = dir.rfind('/');
            if (slash == std::string::npos) return {};
            const std::string candidate = sanitizePkg(dir.substr(slash + 1u));
            return isValidPkg(candidate) ? candidate : std::string{};
        };

        // ── Katman 1: app_data_dir (en güvenilir) ────────────────────────────
        //   /data/user/0/com.example.app  →  com.example.app
        //   /data/user_de/0/com.example.app  →  com.example.app
        std::string pkg_str = extractPkgFromDir(dir_str);

        // ── Katman 2: pkg_data_info_list optional field ───────────────────────
        //   Android 12+ (API 31+)'da bu liste paket adını doğrudan içerir.
        //   Format: [pkgName, seInfo, uid, ...] tekrarlayan gruplar
        if (pkg_str.empty() && args->pkg_data_info_list) {
            const jsize list_len = env->GetArrayLength(*args->pkg_data_info_list);
            if (list_len > 0) {
                auto* jelem = static_cast<jstring>(
                    env->GetObjectArrayElement(*args->pkg_data_info_list, 0));
                if (jelem) {
                    const char* raw = env->GetStringUTFChars(jelem, nullptr);
                    if (raw) {
                        const std::string candidate = sanitizePkg(raw);
                        if (isValidPkg(candidate)) {
                            pkg_str = candidate;
                            LOGI("[Module] preAppSpecialize: pkg from pkg_data_info_list='%s'",
                                 pkg_str.c_str());
                        }
                        env->ReleaseStringUTFChars(jelem, raw);
                    }
                    env->DeleteLocalRef(jelem);
                }
            }
        }

        // ── Katman 3: nice_name (selinux etiketi DEĞİLSE) ────────────────────
        if (pkg_str.empty()) {
            const std::string clean = sanitizePkg(nice_str);
            if (isValidPkg(clean)) {
                pkg_str = clean;
                LOGI("[Module] preAppSpecialize: pkg from nice_name='%s'", pkg_str.c_str());
            }
        }

        // ── Katman 4: uid → /data/system/packages.list ───────────────────────
        //   Root yetkimiz var; packages.list her zaman readable.
        //   Format: "com.example.app 10234 0 /data/data/com.example.app ..."
        if (pkg_str.empty() && args->uid >= 10000) {
            const int target_uid = args->uid;
            FILE* plist = fopen("/data/system/packages.list", "r");
            if (plist) {
                char line[512];
                while (fgets(line, sizeof(line), plist)) {
                    char pname[256] = {};
                    int  puid       = 0;
                    if (sscanf(line, "%255s %d", pname, &puid) == 2 &&
                        puid == target_uid) {
                        const std::string candidate = sanitizePkg(pname);
                        if (isValidPkg(candidate)) {
                            pkg_str = candidate;
                            LOGI("[Module] preAppSpecialize: pkg from packages.list uid=%d → '%s'",
                                 target_uid, pkg_str.c_str());
                            break;
                        }
                    }
                }
                fclose(plist);
            }
        }

        // JNI string'leri serbest bırak
        if (raw_nice) env->ReleaseStringUTFChars(args->nice_name,       raw_nice);
        if (raw_dir)  env->ReleaseStringUTFChars(args->app_data_dir,    raw_dir);
        if (raw_iset) env->ReleaseStringUTFChars(args->instruction_set, raw_iset);

        LOGI("[Module] preAppSpecialize: resolved pkg='%s' | iset='%s' | dir='%s' | nice='%s' | uid=%d | api=%d",
             pkg_str.empty() ? "<empty>" : pkg_str.c_str(),
             iset_str.empty() ? "<empty>" : iset_str.c_str(),
             dir_str.empty()  ? "<empty>" : dir_str.c_str(),
             nice_str.empty() ? "<empty>" : nice_str.c_str(),
             static_cast<int>(args->uid),
             getAndroidApiLevel());

        if (pkg_str.empty()) {
            LOGW("[Module] preAppSpecialize: empty package name -- unloading");
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        if (shouldIgnoreProcess(pkg_str)) {
            LOGI("[Module] preAppSpecialize: system/ignored pkg='%s' -- unloading", pkg_str.c_str());
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const config::DumperConfig fresh_cfg = config::loadConfig();

        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            active_cfg_ = fresh_cfg;
        }

        if (config::isExplicitTarget(fresh_cfg)) {
            // BUG ROOT-CAUSE FIX:
            // Eski kod `fresh_cfg.Target_Game != pkg_str` yapıyordu.
            // pkg_str'de invisible char kalıntısı varsa == dahi başarısız olur.
            // config::isTargetPackage() zaten sanitize edilmiş string karşılaştırması yapıyor.
            // Ek güvence: her iki tarafı da trim ederek karşılaştır.
            const std::string tgt_clean = [&]() {
                std::string s = fresh_cfg.Target_Game;
                s.erase(std::remove_if(s.begin(), s.end(),
                    [](unsigned char c){ return c < 0x20u || c > 0x7Eu; }), s.end());
                const size_t f = s.find_first_not_of(" \t\r\n");
                if (f == std::string::npos) return std::string{};
                return s.substr(f, s.find_last_not_of(" \t\r\n") - f + 1u);
            }();

            const bool matched = (!tgt_clean.empty() && tgt_clean == pkg_str);

            LOGI("[Module] preAppSpecialize: explicit check tgt='%s' pkg='%s' matched=%s",
                 tgt_clean.c_str(), pkg_str.c_str(), matched ? "YES" : "NO");

            if (!matched) {
                LOGI("[Module] preAppSpecialize: explicit target='%s' but pkg='%s' -- no match, unloading",
                     fresh_cfg.Target_Game.c_str(), pkg_str.c_str());
                api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
                return;
            }
            LOGI("[Module] preAppSpecialize: MATCHED explicit target='%s'", pkg_str.c_str());
            prepareTarget(pkg_str, dir_str, fresh_cfg);
            return;
        }

        if (config::isWildcardTarget(fresh_cfg)) {
            if (!process_filter::isThirdPartyApp(pkg_str) && !isInstalledAsUserApp(pkg_str)) {
                LOGI("[Module] preAppSpecialize: wildcard mode but pkg='%s' is not user app -- unloading", pkg_str.c_str());
                api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
                return;
            }
            LOGI("[Module] preAppSpecialize: MATCHED wildcard target='%s'", pkg_str.c_str());
            prepareTarget(pkg_str, dir_str, fresh_cfg);
            return;
        }

        LOGW("[Module] preAppSpecialize: config has no valid target for pkg='%s' -- unloading", pkg_str.c_str());
        api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

    void postAppSpecialize(const rezygisk::AppSpecializeArgs*) override {
        if (!enable_hack_) return;

        // FIX: api pointer kontrolü -- onLoad bug'ından kalan tehlikeye karşı
        if (!api) {
            LOGE("[Module] postAppSpecialize: api is NULL -- aborting");
            return;
        }

        if (isZygoteProcess()) {
            LOGE("[BootGuard] postAppSpecialize called in Zygote context -- cancelled");
            enable_hack_ = false;
            return;
        }

        if (!isApiLevelSupported()) {
            LOGE("[ApiGuard] unsupported API level %d -- cancelled", getAndroidApiLevel());
            enable_hack_ = false;
            return;
        }

        LOGI("[Module] postAppSpecialize: target process confirmed, starting operations");

        installSigsegvHandler();

        config::DumperConfig cfg_copy;
        std::string          out_copy;
        std::string          game_copy;
        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            cfg_copy  = active_cfg_;
            out_copy  = out_dir_;
            game_copy = game_dir_;
        }

        if (cfg_copy.Protected_Breaker) {
            installDlopenHook(out_copy, cfg_copy);
        }

        startConfigWatcher();

        if (s_orig_dlopen && s_orig_dlopen != dlopen) {
            LOGI("postAppSpecialize: dlopen early hook active -- waiting for trigger");
            return;
        }

        std::thread([g = std::move(game_copy),
                     o = std::move(out_copy),
                     c = std::move(cfg_copy)]() mutable {
            hackPrepare(g, o, c);
        }).detach();
    }

private:
    rezygisk::Api*       api         = nullptr;
    JNIEnv*              env         = nullptr;
    bool                 enable_hack_ = false;
    std::string          game_dir_;
    std::string          out_dir_;
    config::DumperConfig active_cfg_;
    config::ConfigWatcher watcher_;
    std::mutex           cfg_mutex_;
    std::atomic<bool>    dump_running_{false};

    void prepareTarget(const std::string& pkg,
                       const std::string& app_data_dir,
                       const config::DumperConfig& cfg)
    {
        LOGI("prepareTarget: pkg=%s  dir=%s  Target=%s  api=%d",
             pkg.c_str(), app_data_dir.c_str(), cfg.Target_Game.c_str(), getAndroidApiLevel());

        enable_hack_ = true;

        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            game_dir_   = app_data_dir;
            out_dir_    = cfg.Output.empty()
                          ? std::string(config::kFallbackOutput)
                          : cfg.Output;
            active_cfg_ = cfg;
        }

        LOGI("TARGET CONFIRMED: %s  [64bit=%d  api=%d  out=%s]",
             pkg.c_str(),
             static_cast<int>(cfg.is_64bit),
             getAndroidApiLevel(),
             out_dir_.c_str());
    }

    void triggerReDump(const config::DumperConfig& new_cfg) {
        bool expected = false;
        if (!dump_running_.compare_exchange_strong(expected, true)) {
            LOGI("hot-reload: dump already running, skipping");
            return;
        }

        const std::string new_out = new_cfg.Output.empty()
                                    ? std::string(config::kFallbackOutput)
                                    : new_cfg.Output;

        if (new_cfg.Protected_Breaker) {
            metadata_hook::resetForReload(new_out);
        }

        const uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (!base) {
            LOGW("hot-reload: libil2cpp.so not yet loaded, using hackPrepare");
            std::string g;
            {
                std::lock_guard<std::mutex> lk(cfg_mutex_);
                g = game_dir_;
            }
            config::DumperConfig c = new_cfg;
            std::thread([g = std::move(g), o = new_out, c = std::move(c), this]() mutable {
                hackPrepare(g, o, c);
                dump_running_.store(false, std::memory_order_release);
            }).detach();
            return;
        }

        config::DumperConfig c = new_cfg;
        std::thread([base, o = new_out, c = std::move(c), this]() mutable {
            runApiInit(base);
            runDump(o, c);
            dump_running_.store(false, std::memory_order_release);
        }).detach();
    }

    void startConfigWatcher() {
        watcher_.start([this](const config::DumperConfig& new_cfg) {
            {
                std::lock_guard<std::mutex> lk(cfg_mutex_);
                active_cfg_ = new_cfg;
            }
            LOGI("hot-reload: config updated Target=%s Cpp=%d Frida=%d Out=%s",
                 new_cfg.Target_Game.c_str(),
                 static_cast<int>(new_cfg.Cpp_Header),
                 static_cast<int>(new_cfg.Frida_Script),
                 new_cfg.Output.c_str());

            if (enable_hack_) {
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
};

REGISTER_REZYGISK_MODULE(EaquelDumperModule)

#if defined(__arm__) || defined(__aarch64__) || defined(__x86_64__) || defined(__i386__)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    if (!vm) return JNI_ERR;

    if (isZygoteProcess()) {
        LOGI("[BootGuard] JNI_OnLoad: Zygote process -- safe return");
        return JNI_VERSION_1_6;
    }

    if (!isApiLevelSupported()) {
        LOGI("[ApiGuard] JNI_OnLoad: API %d not in range [%d-%d] -- safe return",
             getAndroidApiLevel(), config::kAndroidMinApi, config::kAndroidMaxApi);
        return JNI_VERSION_1_6;
    }

    LOGI("[JNI_OnLoad] target process -- starting hack tid=%d", gettid());
    installSigsegvHandler();

    config::DumperConfig cfg = config::loadConfig();
    std::string out = cfg.Output.empty()
                      ? std::string(config::kFallbackOutput)
                      : cfg.Output;
    std::string game;

    if (reserved) {
        const std::string combined(static_cast<const char*>(reserved));
        const auto sep = combined.find('|');
        if (sep != std::string::npos) {
            game = combined.substr(0u, sep);
            const std::string explicit_out = combined.substr(sep + 1u);
            if (!explicit_out.empty()) out = explicit_out;
        } else {
            game = combined;
        }
    }

    if (cfg.Output.empty()) cfg.Output = out;

    std::thread([g = std::move(game),
                 o = std::move(out),
                 c = std::move(cfg)]() mutable {
        hackStart(g, o, c);
    }).detach();

    return JNI_VERSION_1_6;
}
#endif
