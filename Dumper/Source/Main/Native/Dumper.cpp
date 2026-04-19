#define DO_API(r, n, p) r (*n) p = nullptr;
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

static inline const char* safeStr(const char* s, const char* fallback = "") {
    return (s && *s) ? s : fallback;
}

static void runApiInit(uintptr_t base);
static void runDump(const std::string& out_dir, const config::DumperConfig& cfg);

static std::string          s_early_out_dir;
static config::DumperConfig s_early_cfg;
static std::atomic<bool>    s_early_hook_fired{false};
static std::mutex           s_early_cfg_mutex;

namespace hook_engine {

static constexpr size_t kPageSize = 4096;
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
    uintptr_t lo = (near_addr > 0x8000000u) ? (near_addr - 0x8000000u) : 0;
    uintptr_t hi = near_addr + 0x8000000u;
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return nullptr;
    uintptr_t prev_end = lo;
    void* result       = nullptr;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        uintptr_t s = 0, e = 0;
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &s, &e) != 2) continue;
        if (s > hi) break;
        if (s > prev_end && (s - prev_end) >= kPageSize) {
            uintptr_t try_addr = (prev_end + kPageSize - 1) & ~(kPageSize - 1u);
            if (try_addr + kPageSize <= s) {
                void* p = mmap(reinterpret_cast<void*>(try_addr), kPageSize,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
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
static constexpr size_t kHookSize = 16;
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
    if (!tramp_page) { LOGE("hook: trampoline alloc failed"); return false; }

    auto* tramp       = reinterpret_cast<Trampoline*>(tramp_page);
    memcpy(tramp->saved, reinterpret_cast<void*>(tgt), kHookSize);
    tramp->ldr_x17    = 0x58000051u;
    tramp->br_x17     = 0xD61F0220u;
    tramp->return_addr = static_cast<uint64_t>(tgt + kHookSize);
    flushCache(reinterpret_cast<uintptr_t>(tramp), sizeof(Trampoline));

    if (!lockTrampolineRX(tramp_page)) {
        LOGE("hook: trampoline lock RX failed"); return false;
    }

    bool ok = stealthPatchWindow(tgt, kHookSize, [&]() {
        auto* dst = reinterpret_cast<uint8_t*>(tgt);
        uint32_t ldr = 0x58000051u, br = 0xD61F0220u;
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
static constexpr size_t kHookSize = 8;

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
        uintptr_t seg_s = 0, seg_e = 0; char perms[8] = {};
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
            fclose(maps); return true;
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
        LOGI("[MetaHook] Buffer encrypted — running XOR key discovery");
        auto key = entropy::discoverXorKey(buf, size);
        if (key.found && key.score >= 2) {
            plaintext = entropy::decryptBuffer(buf, size, key);
            LOGI("[MetaHook] Decryption applied key_len=%zu score=%d", key.key_len, key.score);
        } else {
            LOGW("[MetaHook] XOR key not confirmed — writing raw buffer");
            plaintext.assign(buf, buf + size);
        }
    } else {
        plaintext.assign(buf, buf + size);
    }
    uint32_t out_magic = 0;
    if (plaintext.size() >= 4) memcpy(&out_magic, plaintext.data(), 4);
    if (out_magic != entropy::kIl2CppMetadataMagic)
        LOGW("[MetaHook] Output magic 0x%08X — may still be encrypted", out_magic);

    std::lock_guard<std::mutex> lk(s_mutex);
    struct stat st{};
    if (stat(s_dump_dir.c_str(), &st) != 0) mkdir(s_dump_dir.c_str(), 0777);
    std::string out_path = s_dump_dir + "/global-metadata.dat";
    int fd = open(out_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { LOGE("[MetaHook] open(%s) failed errno=%d", out_path.c_str(), errno); return; }
    const uint8_t* ptr = plaintext.data();
    size_t left = plaintext.size();
    while (left > 0) {
        ssize_t w = write(fd, ptr, left);
        if (w <= 0) break;
        ptr  += w;
        left -= static_cast<size_t>(w);
    }
    close(fd);
    LOGI("[MetaHook] metadata written (%zu bytes) from %s -> %s",
         plaintext.size(), src_path ? src_path : "?", out_path.c_str());
}

static void* hooked_MetadataLoad(const char* path, size_t* out_size) {
    void* result = s_orig_load(path, out_size);
    bool expected = false;
    if (result && out_size && s_fired.compare_exchange_strong(expected, true)) {
        LOGI("[MetaHook] fired path=%s size=%zu", path ? path : "?", *out_size);
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
    auto syms  = scanner::scanAllSymbols(lib_base);
    uintptr_t loader_addr = 0;
#if defined(__aarch64__)
    loader_addr = syms.metadata_cache_initialize ? syms.metadata_cache_initialize : syms.metadata_loader;
#elif defined(__arm__)
    loader_addr = syms.arm32_metadata_cache_init ? syms.arm32_metadata_cache_init : syms.arm32_metadata_loader;
#endif
    if (!loader_addr) { LOGW("[MetaHook] loader address not found"); return false; }
    bool ok = hook_engine::installInlineHook(
        reinterpret_cast<void*>(loader_addr),
        reinterpret_cast<void*>(&hooked_MetadataLoad),
        reinterpret_cast<void**>(&s_orig_load)
    );
    if (ok) LOGI("[MetaHook] inline hook installed at 0x%" PRIxPTR, loader_addr);
    else    LOGE("[MetaHook] inline hook install failed");
    return ok;
}

static void resetForReload(const std::string& new_dump_dir) {
    std::lock_guard<std::mutex> lk(s_mutex);
    s_dump_dir = new_dump_dir;
    s_fired.store(false, std::memory_order_release);
    LOGI("[MetaHook] reset for hot-reload dump_dir=%s", new_dump_dir.c_str());
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
                Dl_info di{};
                if (dladdr(handle, &di) && di.dli_fbase)
                    base = reinterpret_cast<uintptr_t>(di.dli_fbase);
                if (!base) base = scanner::findLibBase("libil2cpp.so");
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

static bool installDlopenHook(const std::string& out_dir, const config::DumperConfig& cfg) {
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
        LOGI("dlopen hook installed via inline hook");
        return true;
    }
    if (hook_engine::patchGotSlot(
            reinterpret_cast<void*>(dlopen),
            reinterpret_cast<void*>(hooked_dlopen),
            reinterpret_cast<void**>(&s_orig_dlopen)))
    {
        LOGI("dlopen hook installed via GOT patch");
        return true;
    }
    s_orig_dlopen = dlopen;
    LOGW("dlopen hook failed — using poll fallback");
    return false;
}

namespace il2cpp_api {

struct FunctionTable {
    int                    (*init)(const char*)                                                = nullptr;
    Il2CppDomain*          (*domain_get)()                                                     = nullptr;
    const Il2CppAssembly** (*domain_get_assemblies)(const Il2CppDomain*, size_t*)              = nullptr;
    const Il2CppImage*     (*assembly_get_image)(const Il2CppAssembly*)                        = nullptr;
    const char*            (*image_get_name)(const Il2CppImage*)                               = nullptr;
    size_t                 (*image_get_class_count)(const Il2CppImage*)                        = nullptr;
    const Il2CppClass*     (*image_get_class)(const Il2CppImage*, size_t)                      = nullptr;
    const Il2CppType*      (*class_get_type)(Il2CppClass*)                                     = nullptr;
    Il2CppClass*           (*class_from_type)(const Il2CppType*)                               = nullptr;
    const char*            (*class_get_name)(Il2CppClass*)                                     = nullptr;
    const char*            (*class_get_namespace)(Il2CppClass*)                                = nullptr;
    int                    (*class_get_flags)(const Il2CppClass*)                              = nullptr;
    bool                   (*class_is_valuetype)(const Il2CppClass*)                           = nullptr;
    bool                   (*class_is_enum)(const Il2CppClass*)                                = nullptr;
    bool                   (*class_is_abstract)(const Il2CppClass*)                            = nullptr;
    bool                   (*class_is_interface)(const Il2CppClass*)                           = nullptr;
    bool                   (*class_is_generic)(const Il2CppClass*)                             = nullptr;
    bool                   (*class_is_inflated)(const Il2CppClass*)                            = nullptr;
    Il2CppClass*           (*class_get_parent)(Il2CppClass*)                                   = nullptr;
    Il2CppClass*           (*class_get_interfaces)(Il2CppClass*, void**)                       = nullptr;
    Il2CppClass*           (*class_get_nested_types)(Il2CppClass*, void**)                     = nullptr;
    FieldInfo*             (*class_get_fields)(Il2CppClass*, void**)                           = nullptr;
    const PropertyInfo*    (*class_get_properties)(Il2CppClass*, void**)                       = nullptr;
    const MethodInfo*      (*class_get_methods)(Il2CppClass*, void**)                          = nullptr;
    const MethodInfo*      (*class_get_method_from_name)(const Il2CppClass*, const char*, int) = nullptr;
    Il2CppClass*           (*class_from_name)(const Il2CppImage*, const char*, const char*)    = nullptr;
    Il2CppClass*           (*class_from_system_type)(Il2CppReflectionType*)                    = nullptr;
    const Il2CppImage*     (*get_corlib)()                                                     = nullptr;
    int                    (*field_get_flags)(FieldInfo*)                                      = nullptr;
    const char*            (*field_get_name)(FieldInfo*)                                       = nullptr;
    size_t                 (*field_get_offset)(FieldInfo*)                                     = nullptr;
    const Il2CppType*      (*field_get_type)(FieldInfo*)                                       = nullptr;
    void                   (*field_static_get_value)(FieldInfo*, void*)                        = nullptr;
    uint32_t               (*property_get_flags)(PropertyInfo*)                                = nullptr;
    const MethodInfo*      (*property_get_get_method)(PropertyInfo*)                           = nullptr;
    const MethodInfo*      (*property_get_set_method)(PropertyInfo*)                           = nullptr;
    const char*            (*property_get_name)(PropertyInfo*)                                 = nullptr;
    const Il2CppType*      (*method_get_return_type)(const MethodInfo*)                        = nullptr;
    const char*            (*method_get_name)(const MethodInfo*)                               = nullptr;
    uint32_t               (*method_get_param_count)(const MethodInfo*)                        = nullptr;
    const Il2CppType*      (*method_get_param)(const MethodInfo*, uint32_t)                    = nullptr;
    uint32_t               (*method_get_flags)(const MethodInfo*, uint32_t*)                   = nullptr;
    const char*            (*method_get_param_name)(const MethodInfo*, uint32_t)               = nullptr;
    bool                   (*type_is_byref)(const Il2CppType*)                                 = nullptr;
    Il2CppThread*          (*thread_attach)(Il2CppDomain*)                                     = nullptr;
    bool                   (*is_vm_thread)(Il2CppThread*)                                      = nullptr;
    Il2CppString*          (*string_new)(const char*)                                          = nullptr;
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
    if (!handle) LOGW("dlopen RTLD_NOLOAD failed — pattern scan only");

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
            if (s) g_api.domain_get_assemblies = reinterpret_cast<decltype(g_api.domain_get_assemblies)>(s);
        }
    }

    const bool critical_missing =
        !g_api.domain_get || !g_api.domain_get_assemblies ||
        !g_api.image_get_class || !g_api.thread_attach || !g_api.is_vm_thread;

    if (critical_missing) {
        LOGW("critical symbols missing — full adaptive pattern scan");
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
        if (!g_api.image_get_class)       LOGW("image_get_class unresolved — reflection fallback");
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
    auto& api = il2cpp_api::g_api;
    auto name = api.class_get_name ? api.class_get_name(klass) : nullptr;
    return safeStr(name, "object");
}

struct MethodEntry {
    std::string name, return_type, modifier;
    uint64_t rva = 0, va = 0;
    std::vector<std::pair<std::string,std::string>> params;
};

static std::vector<MethodEntry> collectMethods(Il2CppClass* klass) {
    if (!klass) return {};
    auto& api = il2cpp_api::g_api;
    if (!api.class_get_methods) return {};
    std::vector<MethodEntry> entries;
    void* iter = nullptr;
    while (auto method = api.class_get_methods(klass, &iter)) {
        if (!method) continue;
        MethodEntry e;
        e.va  = method->methodPointer ? reinterpret_cast<uint64_t>(method->methodPointer) : 0;
        e.rva = (e.va && il2cpp_api::g_il2cpp_base) ? (e.va - il2cpp_api::g_il2cpp_base) : 0;
        uint32_t iflags = 0;
        if (api.method_get_flags) e.modifier    = buildMethodModifier(api.method_get_flags(method, &iflags));
        if (api.method_get_name)  e.name         = safeStr(api.method_get_name(method));
        if (e.name.empty()) continue;
        if (api.method_get_return_type && api.class_from_type) {
            auto ret = api.method_get_return_type(method);
            if (ret) e.return_type = safeClassName(api.class_from_type(ret));
        }
        if (e.return_type.empty()) e.return_type = "void";
        auto pc = api.method_get_param_count ? api.method_get_param_count(method) : 0u;
        for (uint32_t i = 0; i < pc; ++i) {
            std::string tname = "object", pname = "p" + std::to_string(i);
            if (api.method_get_param && api.class_from_type) {
                auto param = api.method_get_param(method, i);
                if (param) tname = safeClassName(api.class_from_type(param));
            }
            if (api.method_get_param_name) {
                auto pn = api.method_get_param_name(method, i);
                if (pn && *pn) pname = pn;
            }
            e.params.emplace_back(std::move(tname), std::move(pname));
        }
        entries.push_back(std::move(e));
    }
    return entries;
}

static std::string dumpMethods(Il2CppClass* klass) {
    std::stringstream out;
    out << "\n\t// Methods\n";
    for (const auto& e : collectMethods(klass)) {
        out << (e.va ? ("\t// RVA: 0x" + [&]{ std::ostringstream s; s << std::hex << e.rva; return s.str(); }()
                        + " VA: 0x" + [&]{ std::ostringstream s; s << std::hex << e.va; return s.str(); }())
                     : std::string("\t// RVA: 0x0 VA: 0x0")) << "\n";
        out << "\t" << e.modifier << e.return_type << " " << e.name << "(";
        for (size_t i = 0; i < e.params.size(); ++i) {
            if (i > 0) out << ", ";
            out << e.params[i].first << " " << e.params[i].second;
        }
        out << ") { }\n";
    }
    return out.str();
}

static std::string dumpProperties(Il2CppClass* klass) {
    if (!klass) return {};
    auto& api = il2cpp_api::g_api;
    if (!api.class_get_properties) return {};
    std::stringstream out;
    out << "\n\t// Properties\n";
    void* iter = nullptr;
    while (const PropertyInfo* pc = api.class_get_properties(klass, &iter)) {
        if (!pc) continue;
        PropertyInfo* prop = const_cast<PropertyInfo*>(pc);
        auto get_m = api.property_get_get_method ? api.property_get_get_method(prop) : nullptr;
        auto set_m = api.property_get_set_method ? api.property_get_set_method(prop) : nullptr;
        auto pname = api.property_get_name       ? api.property_get_name(prop)       : nullptr;
        if (!pname || !*pname) continue;
        out << "\t";
        Il2CppClass* prop_class = nullptr;
        uint32_t iflags = 0;
        if (get_m && api.method_get_flags && api.method_get_return_type && api.class_from_type) {
            out << buildMethodModifier(api.method_get_flags(get_m, &iflags));
            auto rt = api.method_get_return_type(get_m);
            if (rt) prop_class = api.class_from_type(rt);
        } else if (set_m && api.method_get_flags && api.method_get_param && api.class_from_type) {
            out << buildMethodModifier(api.method_get_flags(set_m, &iflags));
            auto pt = api.method_get_param(set_m, 0);
            if (pt) prop_class = api.class_from_type(pt);
        }
        out << (prop_class ? safeClassName(prop_class) : std::string("object"))
            << " " << pname << " { ";
        if (get_m) out << "get; ";
        if (set_m) out << "set; ";
        out << "}\n";
    }
    return out.str();
}

static std::string dumpFields(Il2CppClass* klass) {
    if (!klass) return {};
    auto& api    = il2cpp_api::g_api;
    if (!api.class_get_fields) return {};
    auto is_enum = api.class_is_enum ? api.class_is_enum(klass) : false;
    std::stringstream out;
    out << "\n\t// Fields\n";
    void* iter = nullptr;
    while (auto field = api.class_get_fields(klass, &iter)) {
        if (!field) continue;
        out << "\t";
        auto attrs = api.field_get_flags ? api.field_get_flags(field) : 0;
        switch (attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK) {
            case FIELD_ATTRIBUTE_PRIVATE:       out << "private ";            break;
            case FIELD_ATTRIBUTE_PUBLIC:        out << "public ";             break;
            case FIELD_ATTRIBUTE_FAMILY:        out << "protected ";          break;
            case FIELD_ATTRIBUTE_ASSEMBLY:
            case FIELD_ATTRIBUTE_FAM_AND_ASSEM: out << "internal ";           break;
            case FIELD_ATTRIBUTE_FAM_OR_ASSEM:  out << "protected internal "; break;
            default: break;
        }
        if (attrs & FIELD_ATTRIBUTE_LITERAL) { out << "const "; }
        else {
            if (attrs & FIELD_ATTRIBUTE_STATIC)    out << "static ";
            if (attrs & FIELD_ATTRIBUTE_INIT_ONLY) out << "readonly ";
        }
        std::string fname = "unknown_field", ftype = "object";
        if (api.field_get_name) { auto n = api.field_get_name(field); if (n && *n) fname = n; }
        if (api.field_get_type && api.class_from_type) {
            auto ft = api.field_get_type(field);
            if (ft) ftype = safeClassName(api.class_from_type(ft));
        }
        out << ftype << " " << fname;
        if ((attrs & FIELD_ATTRIBUTE_LITERAL) && is_enum && api.field_static_get_value) {
            uint64_t val = 0; api.field_static_get_value(field, &val);
            out << " = " << std::dec << val;
        }
        size_t offset = api.field_get_offset ? api.field_get_offset(field) : 0;
        out << "; // 0x" << std::hex << offset << "\n";
    }
    return out.str();
}

static std::string resolveGenericSuffix(Il2CppClass* klass) {
    if (!klass) return {};
    auto& api = il2cpp_api::g_api;
    bool is_gen = (api.class_is_generic  && api.class_is_generic(klass))
               || (api.class_is_inflated && api.class_is_inflated(klass));
    return is_gen ? "<T>" : std::string{};
}

static std::string dumpTypeToString(const Il2CppType* type, int depth = 0);
static std::string dumpTypeToString(const Il2CppType* type, int depth) {
    if (!type || depth > 8) return {};
    auto& api = il2cpp_api::g_api;
    if (!api.class_from_type) return {};
    auto klass = api.class_from_type(type);
    if (!klass) return {};

    auto flags       = api.class_get_flags     ? api.class_get_flags(klass)      : 0;
    auto is_vt       = api.class_is_valuetype  ? api.class_is_valuetype(klass)   : false;
    auto is_enum     = api.class_is_enum       ? api.class_is_enum(klass)        : false;
    auto is_iface    = api.class_is_interface  ? api.class_is_interface(klass)   : false;
    auto is_abstract = api.class_is_abstract   ? api.class_is_abstract(klass)    : false;
    auto ns          = api.class_get_namespace ? safeStr(api.class_get_namespace(klass)) : "";
    auto cn          = api.class_get_name      ? safeStr(api.class_get_name(klass), "UnknownClass") : "UnknownClass";
    auto gen_suffix  = resolveGenericSuffix(klass);

    std::string indent(static_cast<size_t>(depth) * 4, ' ');
    std::stringstream out;
    out << "\n" << indent << "// Namespace: " << ns << "\n";
    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE) out << indent << "[Serializable]\n";
    out << indent;
    switch (flags & TYPE_ATTRIBUTE_VISIBILITY_MASK) {
        case TYPE_ATTRIBUTE_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_PUBLIC:        out << "public ";             break;
        case TYPE_ATTRIBUTE_NOT_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
        case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:      out << "internal ";           break;
        case TYPE_ATTRIBUTE_NESTED_PRIVATE:       out << "private ";            break;
        case TYPE_ATTRIBUTE_NESTED_FAMILY:        out << "protected ";          break;
        case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:  out << "protected internal "; break;
        default: break;
    }
    if ((flags & TYPE_ATTRIBUTE_ABSTRACT) && (flags & TYPE_ATTRIBUTE_SEALED)) out << "static ";
    else if (!is_iface && is_abstract) out << "abstract ";
    else if (flags & TYPE_ATTRIBUTE_SEALED) out << "sealed ";
    if (is_enum)       out << "enum ";
    else if (is_iface) out << "interface ";
    else if (is_vt)    out << "struct ";
    else               out << "class ";
    out << cn << gen_suffix;
    if (api.class_get_parent) {
        auto parent = api.class_get_parent(klass);
        if (parent) {
            auto pname = safeClassName(parent);
            if (pname != "object" && pname != "ValueType" && pname != "Enum")
                out << " : " << pname;
        }
    }
    out << "\n" << indent << "{\n";
    out << dumpFields(klass);
    out << dumpProperties(klass);
    out << dumpMethods(klass);
    if (api.class_get_nested_types) {
        void* niter = nullptr;
        while (auto nested = api.class_get_nested_types(klass, &niter)) {
            if (!nested) continue;
            auto ntype = api.class_get_type ? api.class_get_type(nested) : nullptr;
            if (!ntype) continue;
            auto ns_str = dumpTypeToString(ntype, depth + 1);
            if (!ns_str.empty()) out << ns_str;
        }
    }
    out << indent << "}\n";
    return out.str();
}

namespace output {

static bool ensureDirectory(const char* dir) {
    struct stat st{};
    if (stat(dir, &st) == 0) return S_ISDIR(st.st_mode);
    if (mkdir(dir, 0777) == 0) { chmod(dir, 0777); return true; }
    LOGE("mkdir failed: %s errno=%d", dir, errno);
    return false;
}

static void writeCppHeader(const char* dir,
    const std::vector<std::pair<Il2CppClass*, std::string>>& classes)
{
    auto path = std::string(dir) + "/dump.h";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("failed to open %s", path.c_str()); return; }
    auto& api = il2cpp_api::g_api;
    f << "#pragma once\n#include <cstdint>\n\n";
    for (const auto& [klass, dll] : classes) {
        if (!klass) continue;
        auto cn = api.class_get_name ? api.class_get_name(klass) : nullptr;
        if (!cn || !*cn) continue;
        auto ns = api.class_get_namespace ? api.class_get_namespace(klass) : nullptr;
        f << "// " << dll << " | " << safeStr(ns) << "::" << cn << "\n"
          << "struct " << cn << " {\n";
        void* iter = nullptr;
        if (api.class_get_fields) {
            while (auto field = api.class_get_fields(klass, &iter)) {
                if (!field) continue;
                auto fname   = api.field_get_name   ? api.field_get_name(field)   : nullptr;
                auto foffset = api.field_get_offset ? api.field_get_offset(field) : 0;
                std::string ftname = "void*";
                if (api.field_get_type && api.class_from_type) {
                    auto ft = api.field_get_type(field);
                    if (ft) ftname = safeClassName(api.class_from_type(ft));
                }
                f << "    " << ftname << " " << safeStr(fname, "unknown")
                  << "; // 0x" << std::hex << foffset << "\n";
            }
        }
        for (const auto& m : collectMethods(klass)) {
            if (m.rva == 0) continue;
            f << "    // " << m.modifier << m.return_type << " " << m.name
              << "() RVA=0x" << std::hex << m.rva << "\n";
        }
        f << "};\n\n";
    }
    f.close();
    LOGI("dump.h written to %s", path.c_str());
}

static void writeFridaScript(const char* dir,
    const std::vector<std::pair<Il2CppClass*, std::string>>& classes)
{
    auto path = std::string(dir) + "/dump.js";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("failed to open %s", path.c_str()); return; }
    f << "\"use strict\";\nconst il2cppBase = Module.findBaseAddress(\"libil2cpp.so\");\n\n";
    auto& api = il2cpp_api::g_api;
    for (const auto& [klass, dll] : classes) {
        if (!klass) continue;
        auto cn = api.class_get_name ? api.class_get_name(klass) : nullptr;
        if (!cn || !*cn) continue;
        for (const auto& m : collectMethods(klass)) {
            if (m.rva == 0 || m.name.empty()) continue;
            f << "// " << cn << "::" << m.name << "\n"
              << "Interceptor.attach(il2cppBase.add(0x" << std::hex << m.rva << "), {\n"
              << "    onEnter: function(args) {\n";
            for (size_t i = 0; i < m.params.size(); ++i)
                f << "        // args[" << i << "] -> "
                  << m.params[i].first << " " << m.params[i].second << "\n";
            f << "    },\n    onLeave: function(retval) {\n"
              << "        // retval -> " << m.return_type << "\n"
              << "    }\n});\n\n";
        }
    }
    f.close();
    LOGI("dump.js written to %s", path.c_str());
}

} // namespace output

static void runApiInit(uintptr_t base) {
    il2cpp_api::g_il2cpp_base = static_cast<uint64_t>(base);
    LOGI("il2cpp base: 0x%" PRIx64, il2cpp_api::g_il2cpp_base);
    il2cpp_api::populateFunctionTable(base);
    auto& api = il2cpp_api::g_api;
    if (!api.domain_get_assemblies) { LOGE("il2cpp api init failed"); return; }

    constexpr int kMaxPollMs = 30000;
    constexpr int kSliceMs   = 200;
    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    for (int waited = 0; waited < kMaxPollMs; waited += kSliceMs) {
        if (api.is_vm_thread && api.is_vm_thread(nullptr)) break;
        if (!api.is_vm_thread) break;
        if (efd >= 0) {
            struct pollfd pf = { efd, POLLIN, 0 };
            poll(&pf, 1, kSliceMs);
        }
        LOGI("waiting for il2cpp VM... (%dms / %dms)", waited + kSliceMs, kMaxPollMs);
    }
    if (efd >= 0) ::close(efd);

    if (api.thread_attach && api.domain_get) {
        auto domain = api.domain_get();
        if (domain) api.thread_attach(domain);
    }
}

static void runDump(const std::string& out_dir_str, const config::DumperConfig& cfg) {
    auto& api = il2cpp_api::g_api;
    if (!api.domain_get || !api.domain_get_assemblies) {
        LOGE("runDump: api not initialised"); return;
    }
    const char* out_dir = out_dir_str.c_str();
    LOGI("dump started -> %s", out_dir);

    std::string effective_dir = out_dir_str;
    auto tryDir = [&](const std::string& d) -> bool {
        if (output::ensureDirectory(d.c_str())) { effective_dir = d; return true; }
        return false;
    };

    std::string cfg_out = cfg.Output.empty() ? std::string(config::kFallbackOutput) : cfg.Output;
    if (!tryDir(cfg_out)) {
        if (!tryDir(std::string(config::kFallbackOutput))) {
            if (!tryDir(std::string(config::kFallbackOutputMod))) {
                if (!tryDir(std::string(config::kFallbackOutputKsu))) {
                    LOGE("runDump: no writable dir"); return;
                }
            }
        }
    }
    LOGI("runDump: using %s", effective_dir.c_str());
    const char* dir = effective_dir.c_str();

    size_t ac   = 0;
    auto domain = api.domain_get();
    if (!domain) { LOGE("domain_get null"); return; }
    auto assemblies = api.domain_get_assemblies(domain, &ac);
    if (!assemblies || ac == 0) { LOGE("no assemblies"); return; }

    std::stringstream img_header;
    for (size_t i = 0; i < ac; ++i) {
        if (!assemblies[i]) continue;
        auto img   = api.assembly_get_image ? api.assembly_get_image(assemblies[i]) : nullptr;
        if (!img) continue;
        auto iname = api.image_get_name ? api.image_get_name(img) : nullptr;
        img_header << "// Image " << i << ": " << safeStr(iname) << "\n";
    }

    std::vector<std::string>                          type_outputs;
    std::vector<std::pair<Il2CppClass*, std::string>> class_registry;

    if (api.image_get_class) {
        for (size_t i = 0; i < ac; ++i) {
            if (!assemblies[i]) continue;
            auto img  = api.assembly_get_image ? api.assembly_get_image(assemblies[i]) : nullptr;
            if (!img) continue;
            auto iname = api.image_get_name ? api.image_get_name(img) : nullptr;
            auto dll   = std::string(safeStr(iname));
            auto hdr   = "\n// Dll : " + dll;
            auto cc    = api.image_get_class_count ? api.image_get_class_count(img) : 0;
            for (size_t j = 0; j < cc; ++j) {
                auto klass_c = api.image_get_class(img, j);
                if (!klass_c) continue;
                auto klass = const_cast<Il2CppClass*>(klass_c);
                if (!cfg.include_generic && api.class_is_generic && api.class_is_generic(klass))
                    continue;
                auto type = api.class_get_type ? api.class_get_type(klass) : nullptr;
                if (!type) continue;
                auto dumped = dumpTypeToString(type);
                if (!dumped.empty()) {
                    type_outputs.push_back(hdr + dumped);
                    class_registry.emplace_back(klass, dll);
                }
            }
        }
    } else {
        if (!api.get_corlib || !api.class_from_name || !api.class_get_method_from_name ||
            !api.string_new || !api.class_get_type)
        { LOGE("reflection path: api missing"); return; }
        auto corlib = api.get_corlib();
        if (!corlib) { LOGE("get_corlib null"); return; }
        auto asm_class = api.class_from_name(corlib, "System.Reflection", "Assembly");
        if (!asm_class) { LOGE("Assembly class not found"); return; }
        auto asm_load     = api.class_get_method_from_name(asm_class, "Load",     1);
        auto asm_gettypes = api.class_get_method_from_name(asm_class, "GetTypes", 0);
        if (!asm_load     || !asm_load->methodPointer)     { LOGE("Assembly::Load not found");     return; }
        if (!asm_gettypes || !asm_gettypes->methodPointer) { LOGE("Assembly::GetTypes not found"); return; }
        using Load_fn     = void* (*)(void*, Il2CppString*, void*);
        using GetTypes_fn = Il2CppArray* (*)(void*, void*);
        for (size_t i = 0; i < ac; ++i) {
            if (!assemblies[i]) continue;
            auto img  = api.assembly_get_image ? api.assembly_get_image(assemblies[i]) : nullptr;
            if (!img) continue;
            auto iname = api.image_get_name ? api.image_get_name(img) : nullptr;
            auto dll   = std::string(safeStr(iname));
            auto hdr   = "\n// Dll : " + dll;
            auto dot   = dll.rfind('.');
            auto name  = api.string_new(dll.substr(0, dot).data());
            if (!name) continue;
            auto ref_asm = reinterpret_cast<Load_fn>(asm_load->methodPointer)(nullptr, name, nullptr);
            if (!ref_asm) continue;
            auto ref_types = reinterpret_cast<GetTypes_fn>(asm_gettypes->methodPointer)(ref_asm, nullptr);
            if (!ref_types || ref_types->max_length == 0) continue;
            for (size_t j = 0; j < ref_types->max_length; ++j) {
                if (!ref_types->vector[j]) continue;
                auto klass = api.class_from_system_type(
                    static_cast<Il2CppReflectionType*>(ref_types->vector[j]));
                if (!klass) continue;
                auto type = api.class_get_type(klass);
                if (!type) continue;
                auto dumped = dumpTypeToString(type);
                if (!dumped.empty()) {
                    type_outputs.push_back(hdr + dumped);
                    class_registry.emplace_back(klass, dll);
                }
            }
        }
    }

    {
        auto cs_path = effective_dir + "/dump.cs";
        std::ofstream cs(cs_path);
        if (!cs.is_open()) { LOGE("failed to open %s", cs_path.c_str()); }
        else {
            cs << img_header.str();
            for (const auto& e : type_outputs) cs << e;
            cs.close();
            LOGI("dump.cs written: %zu types -> %s",
                 type_outputs.size(), cs_path.c_str());
        }
    }
    if (cfg.Cpp_Header)   output::writeCppHeader  (dir, class_registry);
    if (cfg.Frida_Script) output::writeFridaScript (dir, class_registry);
    LOGI("Dump complete: %zu classes -> %s", type_outputs.size(), dir);
}

namespace {

constexpr int     kMaxRetries  = 10;
constexpr int64_t kBaseDelayMs = 500;
constexpr int64_t kMaxDelayMs  = 16000;

[[nodiscard]] int64_t exponentialDelay(int attempt) {
    int64_t d = kBaseDelayMs;
    for (int i = 0; i < attempt; ++i) d *= 2;
    return d < kMaxDelayMs ? d : kMaxDelayMs;
}

static void stealthWait(int64_t ms) {
    if (ms <= 0) return;
    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (efd < 0) return;
    struct pollfd pf = { efd, POLLIN, 0 };
    constexpr int64_t kSlice = 250;
    for (int64_t rem = ms; rem > 0; rem -= kSlice)
        poll(&pf, 1, static_cast<int>(rem < kSlice ? rem : kSlice));
    ::close(efd);
}

static void hackStart(
    const std::string& game_dir,
    const std::string& out_dir,
    const config::DumperConfig& cfg
) {
    (void)game_dir;
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (!base) {
            void* h = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
            if (h) {
                Dl_info di{};
                if (dladdr(h, &di) && di.dli_fbase)
                    base = reinterpret_cast<uintptr_t>(di.dli_fbase);
            }
        }
        if (base) {
            if (cfg.Protected_Breaker && !metadata_hook::s_fired.load())
                metadata_hook::install(base, out_dir);
            runApiInit(base);
            runDump(out_dir, cfg);
            return;
        }
        auto delay = exponentialDelay(attempt);
        LOGI("libil2cpp.so not ready attempt=%d delay=%" PRId64 "ms", attempt, delay);
        stealthWait(delay);
    }
    LOGE("libil2cpp.so not found after %d retries tid=%d", kMaxRetries, gettid());
}

static void hackPrepare(
    const std::string& game_dir,
    const std::string& out_dir,
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

class EaquelDumperModule : public rezygisk::ModuleBase {
public:
    void onLoad(rezygisk::Api* api, JNIEnv* env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(rezygisk::AppSpecializeArgs* args) override {
        if (!args) return;
        uint32_t flags = api->getFlags();
        if (flags & static_cast<uint32_t>(rezygisk::StateFlag::PROCESS_ON_DENYLIST)) {
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        const char* pkg = nullptr, *dir = nullptr;
        if (args->nice_name)    pkg = env->GetStringUTFChars(args->nice_name,    nullptr);
        if (args->app_data_dir) dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        preSpecialize(pkg ? pkg : "", dir ? dir : "");
        if (pkg) env->ReleaseStringUTFChars(args->nice_name,    pkg);
        if (dir) env->ReleaseStringUTFChars(args->app_data_dir, dir);
    }

    void postAppSpecialize(const rezygisk::AppSpecializeArgs*) override {
        if (!enable_hack) return;

        installSigsegvHandler();

        if (active_cfg.Protected_Breaker) {
            installDlopenHook(out_dir, active_cfg);
        }

        startConfigWatcher();

        if (s_orig_dlopen && s_orig_dlopen != dlopen) {
            LOGI("postAppSpecialize: early hook active, watcher started");
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
    rezygisk::Api* api         = nullptr;
    JNIEnv*        env         = nullptr;
    bool           enable_hack = false;
    std::string    game_dir, out_dir;
    config::DumperConfig      active_cfg;
    config::ConfigWatcher     watcher_;
    std::mutex                cfg_mutex_;
    std::atomic<bool>         dump_running_{false};

    void triggerReDump(const config::DumperConfig& new_cfg) {
        bool expected = false;
        if (!dump_running_.compare_exchange_strong(expected, true)) {
            LOGI("hot-reload: dump already running, skipping re-trigger");
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
            LOGW("hot-reload: libil2cpp.so not in memory yet, spawning hackStart");
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
            LOGI("hot-reload: config updated Target_Game=%s Cpp_Header=%d Frida=%d Output=%s",
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

    void preSpecialize(const char* pkg, const char* app_data_dir) {
        active_cfg = config::loadConfig();

        LOGI("preSpecialize: nice_name=%s  app_data_dir=%s  Target_Game=%s",
             pkg ? pkg : "null",
             app_data_dir ? app_data_dir : "null",
             active_cfg.Target_Game.c_str());

        bool is_target = false;

        if (active_cfg.Target_Game == "!") {
            is_target = true;
            LOGI("Target_Game=! -> wildcard match for %s", pkg ? pkg : "?");
        } else if (pkg && !active_cfg.Target_Game.empty() && active_cfg.Target_Game == pkg) {
            is_target = true;
        } else if (app_data_dir && !active_cfg.Target_Game.empty() &&
                   strstr(app_data_dir, active_cfg.Target_Game.c_str()) != nullptr) {
            is_target = true;
            LOGI("Target detected via app_data_dir");
        }

        if (!is_target) {
            LOGI("Not target package -> closing module");
            api->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        LOGI("TARGET DETECTED: %s  [64bit=%d]",
             active_cfg.Target_Game.c_str(), (int)active_cfg.is_64bit);
        enable_hack = true;
        game_dir    = app_data_dir ? app_data_dir : "";
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
    installSigsegvHandler();
    config::DumperConfig cfg = config::loadConfig();
    std::string out = cfg.Output.empty() ? std::string(config::kFallbackOutput) : cfg.Output;
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
