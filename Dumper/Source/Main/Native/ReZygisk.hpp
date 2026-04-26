#pragma once

#include <jni.h>
#include <cstdint>

// ReZygisk API sürümü:
//  v4 → Magisk/Zygisk orijinal uyumu
//  v5 → Android 15/16, KernelSU, APatch, Kitsune Magisk uyumu
//       (PROCESS_GRANTED_ROOT / PROCESS_ON_DENYLIST genişletildi)
#define REZYGISK_API_VERSION 5

namespace rezygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

class ModuleBase {
public:
    virtual void onLoad([[maybe_unused]] Api* api,
                        [[maybe_unused]] JNIEnv* env) {}
    virtual void preAppSpecialize([[maybe_unused]] AppSpecializeArgs* args) {}
    virtual void postAppSpecialize([[maybe_unused]] const AppSpecializeArgs* args) {}
    virtual void preServerSpecialize([[maybe_unused]] ServerSpecializeArgs* args) {}
    virtual void postServerSpecialize([[maybe_unused]] const ServerSpecializeArgs* args) {}
    virtual ~ModuleBase() = default;
};

struct AppSpecializeArgs {
    // ── Zorunlu alanlar: tüm Android sürümlerinde garantili ──────────────────
    // UYARI: Alan sırası Zygisk ABI ile BIREBIR eşleşmeli, aksi halde field
    // misread oluşur (örn: instruction_set → app_data_dir olarak okunur → 'arm').
    // Referans: topjohnwu/zygisk-module-sample zygisk.hpp (canonical ABI)
    jint&          uid;
    jint&          gid;
    jintArray&     gids;
    jint&          runtime_flags;
    jobjectArray&  rlimits;        // ← EKSİKTİ! Bu yokken tüm sonraki field'lar kayıyordu.
    jint&          mount_external;
    jstring&       se_info;
    jstring&       nice_name;
    jstring&       instruction_set;
    jstring&       app_data_dir;

    // ── Opsiyonel alanlar: kullanmadan önce NULL kontrolü yapılmalı ──────────
    jintArray*    const fds_to_ignore;           // Android 12+ (Zygisk v3+)
    jboolean*     const is_child_zygote;
    jboolean*     const is_top_app;
    jobjectArray* const pkg_data_info_list;
    jobjectArray* const whitelisted_data_info_list;
    jboolean*     const mount_data_dirs;
    jboolean*     const mount_storage_dirs;

    AppSpecializeArgs() = delete;
};

struct ServerSpecializeArgs {
    jint&      uid;
    jint&      gid;
    jintArray& gids;
    jint&      runtime_flags;
    jlong&     permitted_capabilities;
    jlong&     effective_capabilities;

    ServerSpecializeArgs() = delete;
};

enum Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
};

enum StateFlag : uint32_t {
    PROCESS_GRANTED_ROOT     = (1u << 0),  // Root yetkisi verildi
    PROCESS_ON_DENYLIST      = (1u << 1),  // Zygisk denylist'te
    PROCESS_IS_SYSTEM_SERVER = (1u << 2),  // Android 14+: system_server
    PROCESS_IS_ISOLATED      = (1u << 3),  // Android 14+: isolated process
};

namespace internal {
struct api_table;
template <class T> void entry_impl(api_table*, JNIEnv*);
}

struct Api {
    int      connectCompanion();
    int      getModuleDir();
    void     setOption(Option opt);
    uint32_t getFlags();
    void     hookJniNativeMethods(JNIEnv* env, const char* className,
                                  JNINativeMethod* methods, int numMethods);
    void     pltHookRegister(dev_t dev, ino_t inode, const char* symbol,
                             void* newFunc, void** oldFunc);
    void     pltHookExclude(const char* regex, const char* symbol);
    bool     pltHookCommit();

private:
    internal::api_table* impl;
    template <class T> friend void internal::entry_impl(internal::api_table*, JNIEnv*);
};

#define REGISTER_REZYGISK_MODULE(clazz)                                        \
void rezygisk_module_entry(rezygisk::internal::api_table* table, JNIEnv* env)  \
{                                                                               \
    rezygisk::internal::entry_impl<clazz>(table, env);                         \
}                                                                               \
extern "C" [[gnu::visibility("default")]] [[gnu::used]]                        \
void zygisk_module_entry(rezygisk::internal::api_table* t, JNIEnv* e)          \
{                                                                               \
    rezygisk::internal::entry_impl<clazz>(t, e);                               \
}

#define REGISTER_ZYGISK_MODULE(clazz) REGISTER_REZYGISK_MODULE(clazz)

#define REGISTER_REZYGISK_COMPANION(func) \
void rezygisk_companion_entry(int client) { func(client); }

#define REGISTER_ZYGISK_COMPANION(func) REGISTER_REZYGISK_COMPANION(func)

namespace internal {

struct module_abi {
    long        api_version;
    ModuleBase* _this;

    void (*preAppSpecialize)    (ModuleBase*, AppSpecializeArgs*);
    void (*postAppSpecialize)   (ModuleBase*, const AppSpecializeArgs*);
    void (*preServerSpecialize) (ModuleBase*, ServerSpecializeArgs*);
    void (*postServerSpecialize)(ModuleBase*, const ServerSpecializeArgs*);

    explicit module_abi(ModuleBase* module)
        : api_version(REZYGISK_API_VERSION), _this(module)
    {
        preAppSpecialize     = [](auto s, auto a) { s->preAppSpecialize(a);     };
        postAppSpecialize    = [](auto s, auto a) { s->postAppSpecialize(a);    };
        preServerSpecialize  = [](auto s, auto a) { s->preServerSpecialize(a);  };
        postServerSpecialize = [](auto s, auto a) { s->postServerSpecialize(a); };
    }
};

struct api_table {
    void* _this;
    bool (*registerModule)(api_table*, module_abi*);

    void (*hookJniNativeMethods)(JNIEnv*, const char*, JNINativeMethod*, int);
    void (*pltHookRegister)(dev_t, ino_t, const char*, void*, void**);
    void (*pltHookExclude)(const char*, const char*);
    bool (*pltHookCommit)();

    int      (*connectCompanion)(void*);
    void     (*setOption)(void*, Option);
    int      (*getModuleDir)(void*);
    uint32_t (*getFlags)(void*);
};

template <class T>
void entry_impl(api_table* table, JNIEnv* env) {
    auto* module = new T();
    if (!table->registerModule(table, new module_abi(module)))
        return;
    auto* api = new Api();
    api->impl = table;
    module->onLoad(api, env);
}

}

inline int Api::connectCompanion() {
    return impl->connectCompanion ? impl->connectCompanion(impl->_this) : -1;
}
inline int Api::getModuleDir() {
    return impl->getModuleDir ? impl->getModuleDir(impl->_this) : -1;
}
inline void Api::setOption(Option opt) {
    if (impl->setOption) impl->setOption(impl->_this, opt);
}
inline uint32_t Api::getFlags() {
    return impl->getFlags ? impl->getFlags(impl->_this) : 0u;
}
inline void Api::hookJniNativeMethods(JNIEnv* env, const char* cls,
                                      JNINativeMethod* m, int n) {
    if (impl->hookJniNativeMethods) impl->hookJniNativeMethods(env, cls, m, n);
}
inline void Api::pltHookRegister(dev_t dev, ino_t ino, const char* sym,
                                  void* nf, void** of) {
    if (impl->pltHookRegister) impl->pltHookRegister(dev, ino, sym, nf, of);
}
inline void Api::pltHookExclude(const char* re, const char* sym) {
    if (impl->pltHookExclude) impl->pltHookExclude(re, sym);
}
inline bool Api::pltHookCommit() {
    return impl->pltHookCommit && impl->pltHookCommit();
}

}

namespace zygisk = rezygisk;

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void rezygisk_module_entry(rezygisk::internal::api_table*, JNIEnv*);

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void rezygisk_companion_entry(int);

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void zygisk_module_entry(rezygisk::internal::api_table*, JNIEnv*);

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void zygisk_companion_entry(int);
