#include "Core.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <jni.h>
#include <thread>
#include <sys/mman.h>
#include <linux/unistd.h>
#include <array>
#include <xdl.h>

static void hack_start(const char *game_data_dir) {
    constexpr int kMaxRetries = 10;
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        void *handle = xdl_open("libil2cpp.so", XDL_DEFAULT);
        if (handle) {
            il2cpp_api_init(handle);
            il2cpp_dump(game_data_dir);
            return;
        }
        sleep(1);
    }
    LOGI("libil2cpp.so not found after retries, tid=%d", gettid());
}

static std::string resolveNativeLibDir(JavaVM *jvm) {
    JNIEnv *env = nullptr;
    jvm->AttachCurrentThread(&env, nullptr);

    auto activity_thread_clz = env->FindClass("android/app/ActivityThread");
    if (!activity_thread_clz) {
        LOGE("ActivityThread class not found");
        return {};
    }

    auto current_app_method = env->GetStaticMethodID(
        activity_thread_clz, "currentApplication", "()Landroid/app/Application;");
    if (!current_app_method) {
        LOGE("currentApplication method not found");
        return {};
    }

    auto application = env->CallStaticObjectMethod(activity_thread_clz, current_app_method);
    auto application_clz = env->GetObjectClass(application);
    if (!application_clz) {
        LOGE("application class not found");
        return {};
    }

    auto get_app_info_method = env->GetMethodID(
        application_clz, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    if (!get_app_info_method) {
        LOGE("getApplicationInfo method not found");
        return {};
    }

    auto app_info = env->CallObjectMethod(application, get_app_info_method);
    auto native_lib_dir_field = env->GetFieldID(
        env->GetObjectClass(app_info), "nativeLibraryDir", "Ljava/lang/String;");
    if (!native_lib_dir_field) {
        LOGE("nativeLibraryDir field not found");
        return {};
    }

    auto dir_jstr = reinterpret_cast<jstring>(
        env->GetObjectField(app_info, native_lib_dir_field));
    auto dir_cstr = env->GetStringUTFChars(dir_jstr, nullptr);
    LOGI("native lib dir: %s", dir_cstr);
    std::string result(dir_cstr);
    env->ReleaseStringUTFChars(dir_jstr, dir_cstr);
    return result;
}

static std::string readNativeBridgeProp() {
    auto buf = std::array<char, PROP_VALUE_MAX>{};
    __system_property_get("ro.dalvik.vm.native.bridge", buf.data());
    return {buf.data()};
}

struct NativeBridgeCallbacks {
    uint32_t version;
    void *initialize;
    void *(*loadLibrary)(const char *libpath, int flag);
    void *(*getTrampoline)(void *handle, const char *name, const char *shorty, uint32_t len);
    void *isSupported;
    void *getAppEnv;
    void *isCompatibleWith;
    void *getSignalHandler;
    void *unloadLibrary;
    void *getError;
    void *isPathSupported;
    void *initAnonymousNamespace;
    void *createNamespace;
    void *linkNamespaces;
    void *(*loadLibraryExt)(const char *libpath, int flag, void *ns);
};

static bool loadViaX86NativeBridge(const char *game_data_dir, int api_level,
                                   void *data, size_t length) {
    sleep(5);

    auto libart = dlopen("libart.so", RTLD_NOW);
    auto JNI_GetCreatedJavaVMs = reinterpret_cast<jint (*)(JavaVM **, jsize, jsize *)>(
        dlsym(libart, "JNI_GetCreatedJavaVMs"));
    LOGI("JNI_GetCreatedJavaVMs: %p", JNI_GetCreatedJavaVMs);

    JavaVM *jvm_buf[1];
    jsize num_vms = 0;
    if (JNI_GetCreatedJavaVMs(jvm_buf, 1, &num_vms) != JNI_OK || num_vms == 0) {
        LOGE("JNI_GetCreatedJavaVMs failed");
        return false;
    }
    auto jvm = jvm_buf[0];

    auto lib_dir = resolveNativeLibDir(jvm);
    if (lib_dir.empty()) {
        LOGE("resolveNativeLibDir failed");
        return false;
    }
    if (lib_dir.find("/lib/x86") != std::string::npos) {
        LOGI("x86 native, NativeBridge not required");
        munmap(data, length);
        return false;
    }

    void *nb = dlopen("libhoudini.so", RTLD_NOW);
    if (!nb) {
        auto native_bridge = readNativeBridgeProp();
        LOGI("native bridge lib: %s", native_bridge.data());
        nb = dlopen(native_bridge.data(), RTLD_NOW);
    }
    if (!nb) return false;

    LOGI("native bridge handle: %p", nb);
    auto callbacks = reinterpret_cast<NativeBridgeCallbacks *>(
        dlsym(nb, "NativeBridgeItf"));
    if (!callbacks) return false;

    LOGI("loadLibrary: %p  loadLibraryExt: %p  getTrampoline: %p",
         callbacks->loadLibrary, callbacks->loadLibraryExt, callbacks->getTrampoline);

    int memfd = static_cast<int>(syscall(__NR_memfd_create, "anon", MFD_CLOEXEC));
    ftruncate(memfd, static_cast<off_t>(length));
    void *mem = mmap(nullptr, length, PROT_WRITE, MAP_SHARED, memfd, 0);
    memcpy(mem, data, length);
    munmap(mem, length);
    munmap(data, length);

    char fd_path[PATH_MAX];
    snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", memfd);
    LOGI("memfd path: %s", fd_path);

    void *arm_handle = (api_level >= 26)
        ? callbacks->loadLibraryExt(fd_path, RTLD_NOW, reinterpret_cast<void *>(3))
        : callbacks->loadLibrary(fd_path, RTLD_NOW);

    if (!arm_handle) {
        close(memfd);
        return false;
    }

    LOGI("arm handle: %p", arm_handle);
    auto jni_on_load = reinterpret_cast<void (*)(JavaVM *, void *)>(
        callbacks->getTrampoline(arm_handle, "JNI_OnLoad", nullptr, 0));
    LOGI("JNI_OnLoad: %p", jni_on_load);
    jni_on_load(jvm, const_cast<char *>(game_data_dir));
    return true;
}

void hack_prepare(const char *game_data_dir, void *data, size_t length) {
    LOGI("hack_prepare tid=%d", gettid());
    int api_level = android_get_device_api_level();
    LOGI("api_level=%d", api_level);

#if defined(__i386__) || defined(__x86_64__)
    if (loadViaX86NativeBridge(game_data_dir, api_level, data, length)) return;
#endif
    hack_start(game_data_dir);
}

#if defined(__arm__) || defined(__aarch64__)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    auto game_data_dir = static_cast<const char *>(reserved);
    std::thread(hack_start, game_data_dir).detach();
    return JNI_VERSION_1_6;
}
#endif
