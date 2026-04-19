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
#include <dirent.h>

static inline const char* safeStr(const char* s, const char* fallback = "") {
    return (s && *s) ? s : fallback;
}

static void runApiInit(uintptr_t base);
static void runDump(const std::string& out_dir, const config::DumperConfig& cfg);
static void hackStart(const std::string&, const std::string&, const config::DumperConfig&);

static std::string          s_early_out_dir;
static config::DumperConfig s_early_cfg;
static std::atomic<bool>    s_early_hook_fired{false};
static std::mutex           s_early_cfg_mutex;

static std::atomic<bool>    s_onload_dump_started{false};

[[nodiscard]] static bool isZygoteProcess() noexcept {
    char cmdline[256] = {};
    int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return true;
    ssize_t n = read(fd, cmdline, sizeof(cmdline) - 1);
    close(fd);
    if (n <= 0) return true;
    bool is_zygote = (strncmp(cmdline, "zygote", 6) == 0);
    if (is_zygote) LOGW("[BootGuard] Zygote detected cmdline='%s'", cmdline);
    return is_zygote;
}

[[nodiscard]] static int getAndroidApiLevel() noexcept {
    char prop[PROP_VALUE_MAX] = {};
    __system_property_get("ro.build.version.sdk", prop);
    int api = atoi(prop);
    return (api > 0) ? api : 30;
}

[[nodiscard]] static bool isApiLevelSupported() noexcept {
    int api = getAndroidApiLevel();
    return api >= config::kAndroidMinApi && api <= config::kAndroidMaxApi;
}

[[nodiscard]] static bool isSystemUid(uid_t uid) noexcept {
    return uid < 10000u;
}

[[nodiscard]] static bool isInstalledAsUserApp(std::string_view pkg) noexcept {
    if (pkg.empty()) return false;
    std::string path = "/data/data/" + std::string(pkg);
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) return false;
    return st.st_uid >= 10000u;
}

[[nodiscard]] static bool shouldIgnoreProcess(std::string_view pkg) noexcept {
    if (pkg.empty()) return true;
    if (process_filter::isSystemProcess(pkg)) return true;
    if (process_filter::isSystemPackage(pkg)) return true;
    return false;
}

namespace hook_engine {

static constexpr size_t kPageSize = 4096u;

static uintptr_t alignDown(uintptr_t a) { return a & ~(kPageSize - 1u); }
static uintptr_t alignUp  (uintptr_t a) { return (a + kPageSize - 1u) & ~(kPageSize - 1u); }

static bool stealthPatchWindow(uintptr_t addr, size_t len,
                               const std::function<void()>& patch_fn)
{
    void*  page     = reinterpret_cast<void*>(alignDown(addr));
    size_t page_len = alignUp(len + (addr - alignDown(addr)));

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

static void flushCache(uintptr_t addr, size_t len) {
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr + len));
}

static void* allocTrampolinePage(uintptr_t near_addr) {
    uintptr_t lo = (near_addr > 0x8000000u) ? (near_addr - 0x8000000u) : 0u;
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
            uintptr_t try_addr = (prev_end + kPageSize - 1u) & ~(kPageSize - 1u);
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

static bool lockTrampolineRX(void* page) {
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
    auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[arm64]: trampoline alloc failed"); return false; }

    auto* tramp       = reinterpret_cast<Trampoline*>(tramp_page);
    memcpy(tramp->saved, reinterpret_cast<void*>(tgt), kHookSize);
    tramp->ldr_x17    = 0x58000051u;
    tramp->br_x17     = 0xD61F0220u;
    tramp->return_addr = static_cast<uint64_t>(tgt + kHookSize);
    flushCache(reinterpret_cast<uintptr_t>(tramp), sizeof(Trampoline));

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[arm64]: trampoline lock RX failed"); return false;
    }

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

#elif defined(__arm__)
static constexpr size_t kHookSize = 8u;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) {
    auto tgt_thumb = reinterpret_cast<uintptr_t>(target);
    auto tgt       = tgt_thumb & ~1u;
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[arm32]: trampoline alloc failed"); return false; }

    memcpy(tramp_page, reinterpret_cast<void*>(tgt), kHookSize);
    const uint8_t jback[] = { 0xDF, 0xF8, 0x00, 0xF0 };
    memcpy(tramp_page + kHookSize, jback, 4);
    uint32_t ret_addr = static_cast<uint32_t>(tgt + kHookSize) | 1u;
    memcpy(tramp_page + kHookSize + 4, &ret_addr, 4);
    flushCache(reinterpret_cast<uintptr_t>(tramp_page), kHookSize + 8);

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[arm32]: trampoline lock RX failed"); return false;
    }

    bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        const uint8_t ldr_pc[] = { 0xDF, 0xF8, 0x00, 0xF0 };
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

#elif defined(__x86_64__) || defined(__i386__)
static constexpr size_t kHookSize = 14u;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) {
    auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) return false;

    auto* tramp_page = static_cast<uint8_t*>(allocTrampolinePage(tgt));
    if (!tramp_page) { LOGE("hook[x86]: trampoline alloc failed"); return false; }

    memcpy(tramp_page, reinterpret_cast<void*>(tgt), kHookSize);
    flushCache(reinterpret_cast<uintptr_t>(tramp_page), kHookSize);

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook[x86]: trampoline lock RX failed"); return false;
    }

    bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        auto* dst = reinterpret_cast<uint8_t*>(tgt);
#if defined(__x86_64__)
        dst[0] = 0xFF; dst[1] = 0x25;
        uint32_t rel = 0;
        memcpy(dst + 2, &rel, 4);
        uint64_t hook_addr = reinterpret_cast<uint64_t>(hook);
        memcpy(dst + 6, &hook_addr, 8);
#else
        dst[0] = 0xE9;
        uint32_t rel = static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(hook) - tgt - 5u);
        memcpy(dst + 1, &rel, 4);
#endif
        flushCache(tgt, kHookSize);
    });

    if (!ok) return false;
    if (orig_out) *orig_out = reinterpret_cast<void*>(tramp_page);
    LOGI("hook[x86]: target=%p hook=%p tramp=%p", target, hook, tramp_page);
    return true;
}

#else
[[nodiscard]] bool installInlineHook(void*, void*, void**) { return false; }
#endif

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
        if (perms[1] != 'w') continue;
        if (seg_s < self_base || seg_s > self_base + 0x8000000u) continue;

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

static void installSigsegvHandler() {
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
    if (!data || size < 4) return;

    auto* buf   = reinterpret_cast<const uint8_t*>(data);
    auto  state = entropy::analyzeBuffer(buf, size);
    std::vector<uint8_t> plaintext;

    if (state == entropy::MetadataState::Encrypted) {
        LOGI("[MetaHook] Buffer encrypted -- XOR key scan starting");
        auto key = entropy::discoverXorKey(buf, size);
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
    if (plaintext.size() >= 4) memcpy(&out_magic, plaintext.data(), 4);
    if (out_magic != entropy::kIl2CppMetadataMagic)
        LOGW("[MetaHook] Output magic 0x%08X -- may still be encrypted", out_magic);

    std::lock_guard<std::mutex> lk(s_mutex);
    struct stat st{};
    if (stat(s_dump_dir.c_str(), &st) != 0) mkdir(s_dump_dir.c_str(), 0777);

    std::string out_path = s_dump_dir + "/global-metadata.dat";
    int fd = open(out_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        LOGE("[MetaHook] open(%s) failed errno=%d", out_path.c_str(), errno);
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
#elif defined(__x86_64__) || defined(__i386__)
    loader_addr = syms.metadata_cache_initialize
                  ? syms.metadata_cache_initialize
                  : syms.metadata_loader;
#endif

    if (!loader_addr) { LOGW("[MetaHook] loader address not found"); return false; }

    bool ok = hook_engine::installInlineHook(
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
                int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
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
    s_early_hook_fired.store(false);

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

    LOGW("dlopen hook failed -- polling mode");
    return false;
}

namespace il2cpp_api {

struct FunctionTable {
    bool           (*init)(const char*, int)                                             = nullptr;
    Il2CppDomain*  (*domain_get)()                                                       = nullptr;
    const Il2CppAssembly** (*domain_get_assemblies)(const Il2CppDomain*, size_t*)        = nullptr;
    const Il2CppImage* (*assembly_get_image)(const Il2CppAssembly*)                      = nullptr;
    const char*    (*image_get_name)(const Il2CppImage*)                                 = nullptr;
    size_t         (*image_get_class_count)(const Il2CppImage*)                          = nullptr;
    Il2CppClass*   (*image_get_class)(const Il2CppImage*, size_t)                        = nullptr;
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
    uint32_t       (*field_get_flags)(FieldInfo*)                                        = nullptr;
    const char*    (*field_get_name)(FieldInfo*)                                         = nullptr;
    size_t         (*field_get_offset)(FieldInfo*)                                       = nullptr;
    const Il2CppType* (*field_get_type)(FieldInfo*)                                      = nullptr;
    void           (*field_static_get_value)(FieldInfo*, void*)                          = nullptr;
    uint32_t       (*property_get_flags)(PropertyInfo*)                                  = nullptr;
    const MethodInfo* (*property_get_get_method)(PropertyInfo*)                          = nullptr;
    const MethodInfo* (*property_get_set_method)(PropertyInfo*)                          = nullptr;
    const char*    (*property_get_name)(PropertyInfo*)                                   = nullptr;
    const Il2CppType* (*method_get_return_type)(const MethodInfo*)                       = nullptr;
    const char*    (*method_get_name)(const MethodInfo*)                                 = nullptr;
    uint32_t       (*method_get_param_count)(const MethodInfo*)                          = nullptr;
    const Il2CppType* (*method_get_param)(const MethodInfo*, uint32_t)                   = nullptr;
    uint32_t       (*method_get_flags)(const MethodInfo*, uint32_t*)                     = nullptr;
    const char*    (*method_get_param_name)(const MethodInfo*, uint32_t)                 = nullptr;
    bool           (*type_is_byref)(const Il2CppType*)                                   = nullptr;
    Il2CppThread*  (*thread_attach)(Il2CppDomain*)                                       = nullptr;
    bool           (*is_vm_thread)(Il2CppThread*)                                        = nullptr;
    Il2CppString*  (*string_new)(const char*)                                            = nullptr;
};

static FunctionTable g_api;
static uint64_t      g_il2cpp_base = 0;

template<typename T>
static void resolveSymbol(void* handle, const char* name, T& out) {
    if (!handle || !name) return;
    void* sym = dlsym(handle, name);
    if (sym) out = reinterpret_cast<T>(sym);
    else     LOGW("symbol not resolved: %s", name);
}

template<typename T>
static void applyFallback(uintptr_t addr, T& out) {
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
        if (!g_api.domain_get)            LOGE("FATAL: domain_get unresolved");
        if (!g_api.domain_get_assemblies) LOGE("FATAL: domain_get_assemblies unresolved");
        if (!g_api.image_get_class)       LOGW("image_get_class missing -- reflection fallback");
    }
}

} // namespace il2cpp_api

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

static std::string safeClassName(Il2CppClass* klass) {
    if (!klass) return "object";
    auto& api  = il2cpp_api::g_api;
    auto  name = api.class_get_name ? api.class_get_name(klass) : nullptr;
    return safeStr(name, "object");
}

struct MethodEntry {
    std::string name, return_type, modifier;
    uint64_t    rva = 0, va = 0;
    std::vector<std::pair<std::string, std::string>> params;
};

struct FieldEntry {
    std::string name, type_name, modifier;
    uint64_t    offset = 0;
    bool        is_static = false;
};

struct PropertyEntry {
    std::string name, type_name, modifier;
    bool        has_getter = false, has_setter = false;
};

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

static std::string resolveTypeName(const Il2CppType* t) {
    if (!t) return "void";
    auto& api = il2cpp_api::g_api;

    bool is_byref = api.type_is_byref && api.type_is_byref(t);
    Il2CppClass* klass = api.class_from_type ? api.class_from_type(t) : nullptr;
    std::string  base  = safeClassName(klass);

    if (klass && api.class_is_generic && api.class_is_generic(klass)) {
        base += "<>";
    } else if (klass && api.class_is_inflated && api.class_is_inflated(klass)) {
        base += "<T>";
    }

    if (t->type == IL2CPP_TYPE_SZARRAY || t->type == IL2CPP_TYPE_ARRAY) {
        base += "[]";
    }
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

static void writeCppHeader(const std::string& out_dir,
                           const std::string& image_name,
                           const std::vector<ClassEntry>& classes)
{
    struct stat st{};
    if (stat(out_dir.c_str(), &st) != 0) mkdir(out_dir.c_str(), 0777);

    std::string path = out_dir + "/" + image_name + ".h";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("header write: %s failed", path.c_str()); return; }

    f << "#pragma once\n#include <cstdint>\n\n";

    for (auto& cls : classes) {
        if (!cls.ns.empty()) f << "namespace " << cls.ns << " {\n";
        if (cls.is_interface) f << "struct /* interface */ " << cls.name;
        else if (cls.is_enum) f << "enum class " << cls.name;
        else                  f << "struct " << cls.name;
        if (!cls.parent.empty() && cls.parent != "object")
            f << " : public " << cls.parent;
        f << " {\n";

        for (auto& fld : cls.fields) {
            f << "    " << fld.modifier << fld.type_name << " " << fld.name
              << "; // 0x" << std::hex << fld.offset << std::dec << "\n";
        }
        for (auto& m : cls.methods) {
            f << "    " << m.modifier << m.return_type << " " << m.name << "(";
            for (size_t i = 0; i < m.params.size(); ++i) {
                if (i) f << ", ";
                f << m.params[i].first << " " << m.params[i].second;
            }
            f << "); // RVA: 0x" << std::hex << m.rva << std::dec << "\n";
        }
        f << "};\n";
        if (!cls.ns.empty()) f << "} // namespace " << cls.ns << "\n";
        f << "\n";
    }
    LOGI("C++ header written: %s (%zu classes)", path.c_str(), classes.size());
}

static void writeFridaScript(const std::string& out_dir,
                             const std::string& image_name,
                             const std::vector<ClassEntry>& classes)
{
    struct stat st{};
    if (stat(out_dir.c_str(), &st) != 0) mkdir(out_dir.c_str(), 0777);

    std::string path = out_dir + "/" + image_name + ".js";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("frida script write: %s failed", path.c_str()); return; }

    f << "var il2cpp_base = Module.findBaseAddress('libil2cpp.so');\n"
      << "if (!il2cpp_base) throw new Error('libil2cpp.so not found');\n\n";

    for (auto& cls : classes) {
        for (auto& m : cls.methods) {
            if (!m.rva) continue;
            f << "var addr_" << cls.name << "_" << m.name
              << " = il2cpp_base.add(0x" << std::hex << m.rva << std::dec << ");\n"
              << "Interceptor.attach(addr_" << cls.name << "_" << m.name << ", {\n"
              << "    onEnter: function(args) {},\n"
              << "    onLeave: function(retval) {}\n"
              << "});\n\n";
        }
    }
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
    if (!assemblies || !count) {
        LOGE("runDump: assembly list empty"); return;
    }

    LOGI("runDump: %zu assemblies found", count);

    struct stat st{};
    if (stat(out_dir.c_str(), &st) != 0) mkdir(out_dir.c_str(), 0777);

    for (size_t ai = 0; ai < count; ++ai) {
        const Il2CppAssembly* asm_ptr = assemblies[ai];
        if (!asm_ptr) continue;

        const Il2CppImage* image = api.assembly_get_image
                                   ? api.assembly_get_image(asm_ptr) : nullptr;
        if (!image) continue;

        const char* img_name = api.image_get_name ? api.image_get_name(image) : nullptr;
        std::string name     = safeStr(img_name, "unknown");
        if (name.size() > 4 && name.substr(name.size() - 4) == ".dll")
            name = name.substr(0, name.size() - 4);

        size_t cls_count = api.image_get_class_count
                           ? api.image_get_class_count(image) : 0;
        LOGI("image[%zu]: %s (%zu classes)", ai, name.c_str(), cls_count);

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

        if (cfg.Cpp_Header)   writeCppHeader  (out_dir, name, classes);
        if (cfg.Frida_Script) writeFridaScript (out_dir, name, classes);
    }
    LOGI("runDump: complete  out_dir=%s", out_dir.c_str());
}

namespace {

static int64_t exponentialDelay(int attempt) {
    int64_t ms = 200LL * (1LL << std::min(attempt, 5));
    return std::min(ms, static_cast<int64_t>(2000));
}

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

static void hackStart(
    const std::string&          game_dir,
    const std::string&          out_dir,
    const config::DumperConfig& cfg
) {
    if (isZygoteProcess()) {
        LOGE("[BootGuard] hackStart called from Zygote -- aborting");
        return;
    }

    if (!isApiLevelSupported()) {
        int api = getAndroidApiLevel();
        LOGE("[ApiGuard] Android API %d not in supported range [%d-%d] -- aborting",
             api, config::kAndroidMinApi, config::kAndroidMaxApi);
        return;
    }

    LOGI("hackStart tid=%d  game=%s  out=%s  api=%d",
         gettid(), game_dir.c_str(), out_dir.c_str(), getAndroidApiLevel());

    constexpr int     kMaxRetries  = 20;
    constexpr int64_t kMaxTotalMs  = 30'000;
    int64_t           elapsed_ms   = 0;

    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (base) {
            LOGI("libil2cpp.so found base=0x%" PRIxPTR " attempt=%d", base, attempt);

            if (cfg.Protected_Breaker && !metadata_hook::s_fired.load())
                metadata_hook::install(base, out_dir);

            runApiInit(base);
            runDump(out_dir, cfg);
            return;
        }

        auto delay = exponentialDelay(attempt);
        elapsed_ms += delay;
        if (elapsed_ms >= kMaxTotalMs) {
            LOGE("[BootGuard] TIMEOUT: libil2cpp.so not found in 30s -- stopping");
            return;
        }

        LOGI("libil2cpp.so not ready attempt=%d delay=%" PRId64 "ms elapsed=%" PRId64 "ms",
             attempt, delay, elapsed_ms);
        stealthWait(delay);
    }
    LOGE("libil2cpp.so not found after %d attempts tid=%d", kMaxRetries, gettid());
}

static void hackPrepare(
    const std::string&          game_dir,
    const std::string&          out_dir,
    const config::DumperConfig& cfg
) {
    LOGI("hack_prepare tid=%d api=%d", gettid(), getAndroidApiLevel());
#if defined(__arm__)
    LOGI("mode: ARMv7/Thumb-2 (32-bit)");
#elif defined(__aarch64__)
    LOGI("mode: AArch64 (64-bit)");
#elif defined(__x86_64__)
    LOGI("mode: x86_64 (64-bit emulator)");
#elif defined(__i386__)
    LOGI("mode: x86 (32-bit emulator)");
#endif
    hackStart(game_dir, out_dir, cfg);
}

static void launchOnLoadDump(const config::DumperConfig& cfg) {
    bool expected = false;
    if (!s_onload_dump_started.compare_exchange_strong(expected, true)) {
        LOGI("[onLoad-Dump] already launched, skipping");
        return;
    }

    std::string out = cfg.Output.empty()
                      ? std::string(config::kFallbackOutput)
                      : cfg.Output;

    LOGI("[onLoad-Dump] launching dump thread Target=%s Out=%s", cfg.Target_Game.c_str(), out.c_str());

    installSigsegvHandler();

    if (cfg.Protected_Breaker) {
        std::lock_guard<std::mutex> lk(s_early_cfg_mutex);
        s_early_out_dir = out;
        s_early_cfg     = cfg;
    }

    if (cfg.Protected_Breaker) {
        installDlopenHook(out, cfg);
    }

    config::DumperConfig c = cfg;
    std::thread([out = std::move(out), c = std::move(c)]() mutable {
        hackPrepare("", out, c);
    }).detach();
}

} // anonymous namespace

class EaquelDumperModule : public rezygisk::ModuleBase {
public:

    void onLoad(rezygisk::Api* api_, JNIEnv* env_) override {
        this->api = api_;
        this->env = env_;
        LOGI("[Module] onLoad: registered (Zygote safe mode)");

        auto cfg = config::loadConfig();
        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            active_cfg = cfg;
        }

        if (!config::isExplicitTarget(cfg) && !config::isWildcardTarget(cfg)) {
            LOGW("[onLoad] No valid target configured -- waiting for preAppSpecialize");
            return;
        }

        LOGI("[onLoad] Config valid Target=%s -- launching zygote-based dump immediately", cfg.Target_Game.c_str());
        launchOnLoadDump(cfg);
    }

    void preAppSpecialize(rezygisk::AppSpecializeArgs* args) override {
        if (!args) {
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const char* pkg = nullptr;
        const char* dir = nullptr;
        if (args->nice_name)    pkg = env->GetStringUTFChars(args->nice_name,    nullptr);
        if (args->app_data_dir) dir = env->GetStringUTFChars(args->app_data_dir, nullptr);

        std::string pkg_str = pkg ? pkg : "";
        std::string dir_str = dir ? dir : "";

        if (pkg) env->ReleaseStringUTFChars(args->nice_name,    pkg);
        if (dir) env->ReleaseStringUTFChars(args->app_data_dir, dir);

        if (shouldIgnoreProcess(pkg_str)) {
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        config::DumperConfig fresh_cfg = config::loadConfig();
        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            active_cfg = fresh_cfg;
        }

        if (config::isExplicitTarget(fresh_cfg)) {
            if (fresh_cfg.Target_Game != pkg_str) {
                api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
                return;
            }
            preSpecialize(pkg_str, dir_str);
            return;
        }

        if (config::isWildcardTarget(fresh_cfg)) {
            if (!process_filter::isThirdPartyApp(pkg_str) && !isInstalledAsUserApp(pkg_str)) {
                api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
                return;
            }
            preSpecialize(pkg_str, dir_str);
            return;
        }

        api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

    void postAppSpecialize(const rezygisk::AppSpecializeArgs*) override {
        if (!enable_hack) return;

        if (isZygoteProcess()) {
            LOGE("[BootGuard] postAppSpecialize from Zygote -- cancelled");
            return;
        }

        if (!isApiLevelSupported()) {
            LOGE("[ApiGuard] unsupported API level -- cancelled");
            return;
        }

        LOGI("[Module] postAppSpecialize: starting hack in target process");

        installSigsegvHandler();

        if (active_cfg.Protected_Breaker) {
            installDlopenHook(out_dir, active_cfg);
        }

        startConfigWatcher();

        if (s_orig_dlopen && s_orig_dlopen != dlopen) {
            LOGI("postAppSpecialize: dlopen early hook active");
            return;
        }

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
    bool                 enable_hack = false;
    std::string          game_dir, out_dir;
    config::DumperConfig active_cfg;
    config::ConfigWatcher watcher_;
    std::mutex            cfg_mutex_;
    std::atomic<bool>     dump_running_{false};

    void triggerReDump(const config::DumperConfig& new_cfg) {
        bool expected = false;
        if (!dump_running_.compare_exchange_strong(expected, true)) {
            LOGI("hot-reload: dump already running, skipping");
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
            LOGW("hot-reload: libil2cpp.so not yet loaded, starting hackStart");
            std::string g = game_dir;
            config::DumperConfig c = new_cfg;
            std::thread([g = std::move(g), o = new_out, c = std::move(c),
                         this]() mutable {
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
                active_cfg  = new_cfg;
                s_early_cfg = new_cfg;
            }
            LOGI("hot-reload: config updated Target=%s Cpp=%d Frida=%d Out=%s",
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

    void preSpecialize(const std::string& pkg, const std::string& app_data_dir) {
        LOGI("preSpecialize: pkg=%s  dir=%s  Target=%s",
             pkg.c_str(), app_data_dir.c_str(), active_cfg.Target_Game.c_str());

        bool is_target = false;

        if (config::isWildcardTarget(active_cfg)) {
            if (process_filter::isThirdPartyApp(pkg) || isInstalledAsUserApp(pkg)) {
                is_target = true;
                LOGI("Target_Game=! wildcard match for third-party app %s", pkg.c_str());
            }
        } else if (config::isExplicitTarget(active_cfg)) {
            if (active_cfg.Target_Game == pkg) {
                is_target = true;
                LOGI("Explicit target match: %s", pkg.c_str());
            } else if (!app_data_dir.empty() &&
                       strstr(app_data_dir.c_str(), active_cfg.Target_Game.c_str()) != nullptr) {
                is_target = true;
                LOGI("Explicit target detected via app_data_dir");
            }
        }

        if (!is_target) {
            LOGI("Not target package -- closing module");
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        LOGI("TARGET DETECTED: %s  [64bit=%d  api=%d]",
             active_cfg.Target_Game.c_str(), (int)active_cfg.is_64bit, getAndroidApiLevel());

        enable_hack = true;
        game_dir    = app_data_dir;
        out_dir     = active_cfg.Output.empty()
                      ? std::string(config::kFallbackOutput)
                      : active_cfg.Output;

        LOGI("output dir: %s", out_dir.c_str());
    }
};

REGISTER_REZYGISK_MODULE(EaquelDumperModule)

#if defined(__arm__) || defined(__aarch64__)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    if (!vm) return JNI_ERR;

    if (isZygoteProcess()) {
        LOGI("[BootGuard] JNI_OnLoad: Zygote process -- safe return");
        return JNI_VERSION_1_6;
    }

    if (!isApiLevelSupported()) {
        LOGI("[ApiGuard] JNI_OnLoad: unsupported API -- safe return");
        return JNI_VERSION_1_6;
    }

    LOGI("[JNI_OnLoad] target process -- starting hack");
    installSigsegvHandler();

    config::DumperConfig cfg = config::loadConfig();
    std::string out = cfg.Output.empty()
                      ? std::string(config::kFallbackOutput)
                      : cfg.Output;
    std::string game;

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

    std::thread([g = std::move(g), o = std::move(o), c = std::move(c)]() mutable {
        hackStart(g, o, c);
    }).detach();

    return JNI_VERSION_1_6;
}
#endif
