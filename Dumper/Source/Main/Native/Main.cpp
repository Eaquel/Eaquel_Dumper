#include <cstring>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cinttypes>
#include "Zygisk.hpp"
#include "Target.h"

#define DO_API(r, n, p) r (*n) p = nullptr;
#define DO_API_NO_RETURN(r, n, p) r (*n) p = nullptr;
#include "Core.h"
#undef DO_API
#undef DO_API_NO_RETURN

void hack_prepare(const char *game_data_dir, void *data, size_t length);

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class EaquelDumperModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        preSpecialize(package_name, app_data_dir);
        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (!enable_hack) return;
        std::thread hack_thread(hack_prepare, game_data_dir, bridge_data, bridge_length);
        hack_thread.detach();
    }

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
    bool enable_hack = false;
    char *game_data_dir = nullptr;
    void *bridge_data = nullptr;
    size_t bridge_length = 0;

    void preSpecialize(const char *package_name, const char *app_data_dir) {
        if (strcmp(package_name, GamePackageName) != 0) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        LOGI("target process detected: %s", package_name);
        enable_hack = true;
        game_data_dir = new char[strlen(app_data_dir) + 1];
        strcpy(game_data_dir, app_data_dir);

#if defined(__i386__) || defined(__x86_64__)
        loadArmBridgePayload();
#endif
    }

    void loadArmBridgePayload() {
        const char *path = nullptr;
#if defined(__i386__)
        path = "zygisk/armeabi-v7a.so";
#elif defined(__x86_64__)
        path = "zygisk/arm64-v8a.so";
#else
        return;
#endif
        int dirfd = api->getModuleDir();
        int fd = openat(dirfd, path, O_RDONLY);
        if (fd == -1) {
            LOGW("arm bridge payload not found at %s", path);
            return;
        }
        struct stat sb{};
        fstat(fd, &sb);
        bridge_length = static_cast<size_t>(sb.st_size);
        bridge_data = mmap(nullptr, bridge_length, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
    }
};

REGISTER_ZYGISK_MODULE(EaquelDumperModule)
