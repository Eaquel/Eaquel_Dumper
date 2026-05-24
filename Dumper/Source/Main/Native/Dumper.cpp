
#include <android/log.h>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <cmath>
#include <optional>
#include <expected>
#include <array>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <span>
#include <map>
#include <set>
#include <source_location>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <sys/system_properties.h>
#include <sys/uio.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <link.h>
#include <elf.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

namespace eaquel::log {

inline constexpr std::string_view kTag = "Eaquel";

template <typename... Args>
[[gnu::always_inline]] inline void logDebug(
    [[maybe_unused]] const char* fmt,
    [[maybe_unused]] Args&&... args) noexcept
{
#if defined(DEBUG_BUILD)
    __android_log_print(ANDROID_LOG_DEBUG,
                        kTag.data(), fmt, static_cast<Args&&>(args)...);
#endif
}

template <typename... Args>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wformat-security"

[[gnu::always_inline]] inline void logInfo(const char* fmt, Args&&... args) noexcept {
    __android_log_print(ANDROID_LOG_INFO,
                        kTag.data(), fmt, static_cast<Args&&>(args)...);
}

template <typename... Args>
[[gnu::always_inline]] inline void logWarn(const char* fmt, Args&&... args) noexcept {
    __android_log_print(ANDROID_LOG_WARN,
                        kTag.data(), fmt, static_cast<Args&&>(args)...);
}

template <typename... Args>
[[gnu::always_inline]] inline void logError(const char* fmt, Args&&... args) noexcept {
    __android_log_print(ANDROID_LOG_ERROR,
                        kTag.data(), fmt, static_cast<Args&&>(args)...);
}

#pragma clang diagnostic pop

}

#define LOGD(...) ::eaquel::log::logDebug(__VA_ARGS__)
#define LOGI(...) ::eaquel::log::logInfo(__VA_ARGS__)
#define LOGW(...) ::eaquel::log::logWarn(__VA_ARGS__)
#define LOGE(...) ::eaquel::log::logError(__VA_ARGS__)

namespace eaquel {

struct ScopedFd {
    int fd = -1;

    constexpr ScopedFd() noexcept = default;
    explicit constexpr ScopedFd(int f) noexcept : fd(f) {}
    ~ScopedFd() noexcept { reset(); }

    ScopedFd(const ScopedFd&)            = delete;
    ScopedFd& operator=(const ScopedFd&) = delete;

    ScopedFd(ScopedFd&& o) noexcept : fd(o.fd) { o.fd = -1; }
    ScopedFd& operator=(ScopedFd&& o) noexcept {
        if (this != &o) { reset(); fd = o.fd; o.fd = -1; }
        return *this;
    }

    [[nodiscard]] bool valid() const noexcept { return fd >= 0; }
    [[nodiscard]] int  get()   const noexcept { return fd; }

    void reset() noexcept {
        if (fd >= 0) { ::close(fd); fd = -1; }
    }
};

}

inline constexpr uint32_t FIELD_ATTRIBUTE_FIELD_ACCESS_MASK    = 0x0007u;
inline constexpr uint32_t FIELD_ATTRIBUTE_COMPILER_CONTROLLED  = 0x0000u;
inline constexpr uint32_t FIELD_ATTRIBUTE_PRIVATE              = 0x0001u;
inline constexpr uint32_t FIELD_ATTRIBUTE_FAM_AND_ASSEM        = 0x0002u;
inline constexpr uint32_t FIELD_ATTRIBUTE_ASSEMBLY             = 0x0003u;
inline constexpr uint32_t FIELD_ATTRIBUTE_FAMILY               = 0x0004u;
inline constexpr uint32_t FIELD_ATTRIBUTE_FAM_OR_ASSEM         = 0x0005u;
inline constexpr uint32_t FIELD_ATTRIBUTE_PUBLIC               = 0x0006u;
inline constexpr uint32_t FIELD_ATTRIBUTE_STATIC               = 0x0010u;
inline constexpr uint32_t FIELD_ATTRIBUTE_INIT_ONLY            = 0x0020u;
inline constexpr uint32_t FIELD_ATTRIBUTE_LITERAL              = 0x0040u;
inline constexpr uint32_t FIELD_ATTRIBUTE_NOT_SERIALIZED       = 0x0080u;
inline constexpr uint32_t FIELD_ATTRIBUTE_SPECIAL_NAME         = 0x0200u;
inline constexpr uint32_t FIELD_ATTRIBUTE_PINVOKE_IMPL         = 0x2000u;
inline constexpr uint32_t FIELD_ATTRIBUTE_RESERVED_MASK        = 0x9500u;
inline constexpr uint32_t FIELD_ATTRIBUTE_RT_SPECIAL_NAME      = 0x0400u;
inline constexpr uint32_t FIELD_ATTRIBUTE_HAS_FIELD_MARSHAL    = 0x1000u;
inline constexpr uint32_t FIELD_ATTRIBUTE_HAS_DEFAULT          = 0x8000u;
inline constexpr uint32_t FIELD_ATTRIBUTE_HAS_FIELD_RVA        = 0x0100u;

inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_CODE_TYPE_MASK      = 0x0003u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_IL                  = 0x0000u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_NATIVE              = 0x0001u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_OPTIL               = 0x0002u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_RUNTIME             = 0x0003u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_MANAGED_MASK        = 0x0004u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_UNMANAGED           = 0x0004u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_MANAGED             = 0x0000u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_FORWARD_REF         = 0x0010u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_PRESERVE_SIG        = 0x0080u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_INTERNAL_CALL       = 0x1000u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_SYNCHRONIZED        = 0x0020u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_NOINLINING          = 0x0008u;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_MAX_METHOD_IMPL_VAL = 0xffffu;

inline constexpr uint32_t METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK   = 0x0007u;
inline constexpr uint32_t METHOD_ATTRIBUTE_COMPILER_CONTROLLED  = 0x0000u;
inline constexpr uint32_t METHOD_ATTRIBUTE_PRIVATE              = 0x0001u;
inline constexpr uint32_t METHOD_ATTRIBUTE_FAM_AND_ASSEM        = 0x0002u;
inline constexpr uint32_t METHOD_ATTRIBUTE_ASSEM                = 0x0003u;
inline constexpr uint32_t METHOD_ATTRIBUTE_FAMILY               = 0x0004u;
inline constexpr uint32_t METHOD_ATTRIBUTE_FAM_OR_ASSEM         = 0x0005u;
inline constexpr uint32_t METHOD_ATTRIBUTE_PUBLIC               = 0x0006u;
inline constexpr uint32_t METHOD_ATTRIBUTE_STATIC               = 0x0010u;
inline constexpr uint32_t METHOD_ATTRIBUTE_FINAL                = 0x0020u;
inline constexpr uint32_t METHOD_ATTRIBUTE_VIRTUAL              = 0x0040u;
inline constexpr uint32_t METHOD_ATTRIBUTE_HIDE_BY_SIG          = 0x0080u;
inline constexpr uint32_t METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK   = 0x0100u;
inline constexpr uint32_t METHOD_ATTRIBUTE_REUSE_SLOT           = 0x0000u;
inline constexpr uint32_t METHOD_ATTRIBUTE_NEW_SLOT             = 0x0100u;
inline constexpr uint32_t METHOD_ATTRIBUTE_STRICT               = 0x0200u;
inline constexpr uint32_t METHOD_ATTRIBUTE_ABSTRACT             = 0x0400u;
inline constexpr uint32_t METHOD_ATTRIBUTE_SPECIAL_NAME         = 0x0800u;
inline constexpr uint32_t METHOD_ATTRIBUTE_PINVOKE_IMPL         = 0x2000u;
inline constexpr uint32_t METHOD_ATTRIBUTE_UNMANAGED_EXPORT     = 0x0008u;
inline constexpr uint32_t METHOD_ATTRIBUTE_RESERVED_MASK        = 0xd000u;
inline constexpr uint32_t METHOD_ATTRIBUTE_RT_SPECIAL_NAME      = 0x1000u;
inline constexpr uint32_t METHOD_ATTRIBUTE_HAS_SECURITY         = 0x4000u;
inline constexpr uint32_t METHOD_ATTRIBUTE_REQUIRE_SEC_OBJECT   = 0x8000u;

inline constexpr uint32_t TYPE_ATTRIBUTE_VISIBILITY_MASK        = 0x00000007u;
inline constexpr uint32_t TYPE_ATTRIBUTE_NOT_PUBLIC             = 0x00000000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_PUBLIC                 = 0x00000001u;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_PUBLIC          = 0x00000002u;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_PRIVATE         = 0x00000003u;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_FAMILY          = 0x00000004u;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_ASSEMBLY        = 0x00000005u;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM   = 0x00000006u;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM    = 0x00000007u;
inline constexpr uint32_t TYPE_ATTRIBUTE_LAYOUT_MASK            = 0x00000018u;
inline constexpr uint32_t TYPE_ATTRIBUTE_AUTO_LAYOUT            = 0x00000000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_SEQUENTIAL_LAYOUT      = 0x00000008u;
inline constexpr uint32_t TYPE_ATTRIBUTE_EXPLICIT_LAYOUT        = 0x00000010u;
inline constexpr uint32_t TYPE_ATTRIBUTE_CLASS_SEMANTIC_MASK    = 0x00000020u;
inline constexpr uint32_t TYPE_ATTRIBUTE_CLASS                  = 0x00000000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_INTERFACE              = 0x00000020u;
inline constexpr uint32_t TYPE_ATTRIBUTE_ABSTRACT               = 0x00000080u;
inline constexpr uint32_t TYPE_ATTRIBUTE_SEALED                 = 0x00000100u;
inline constexpr uint32_t TYPE_ATTRIBUTE_SPECIAL_NAME           = 0x00000400u;
inline constexpr uint32_t TYPE_ATTRIBUTE_IMPORT                 = 0x00001000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_SERIALIZABLE           = 0x00002000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_STRING_FORMAT_MASK     = 0x00030000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_ANSI_CLASS             = 0x00000000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_UNICODE_CLASS          = 0x00010000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_AUTO_CLASS             = 0x00020000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_BEFORE_FIELD_INIT      = 0x00100000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_FORWARDER              = 0x00200000u;
inline constexpr uint32_t TYPE_ATTRIBUTE_RESERVED_MASK          = 0x00040800u;
inline constexpr uint32_t TYPE_ATTRIBUTE_RT_SPECIAL_NAME        = 0x00000800u;
inline constexpr uint32_t TYPE_ATTRIBUTE_HAS_SECURITY           = 0x00040000u;

inline constexpr uint32_t PARAM_ATTRIBUTE_IN                = 0x0001u;
inline constexpr uint32_t PARAM_ATTRIBUTE_OUT               = 0x0002u;
inline constexpr uint32_t PARAM_ATTRIBUTE_OPTIONAL          = 0x0010u;
inline constexpr uint32_t PARAM_ATTRIBUTE_RESERVED_MASK     = 0xf000u;
inline constexpr uint32_t PARAM_ATTRIBUTE_HAS_DEFAULT       = 0x1000u;
inline constexpr uint32_t PARAM_ATTRIBUTE_HAS_FIELD_MARSHAL = 0x2000u;
inline constexpr uint32_t PARAM_ATTRIBUTE_UNUSED            = 0xcfe0u;

inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_NON_VARIANT                        = 0x00u;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_COVARIANT                          = 0x01u;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_CONTRAVARIANT                      = 0x02u;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_VARIANCE_MASK                      = 0x03u;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_REFERENCE_TYPE_CONSTRAINT          = 0x04u;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_NOT_NULLABLE_VALUE_TYPE_CONSTRAINT = 0x08u;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_DEFAULT_CONSTRUCTOR_CONSTRAINT     = 0x10u;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_SPECIAL_CONSTRAINT_MASK            = 0x1Cu;

inline constexpr uint32_t ASSEMBLYREF_FULL_PUBLIC_KEY_FLAG             = 0x00000001u;
inline constexpr uint32_t ASSEMBLYREF_RETARGETABLE_FLAG                = 0x00000100u;
inline constexpr uint32_t ASSEMBLYREF_ENABLEJITCOMPILE_TRACKING_FLAG   = 0x00008000u;
inline constexpr uint32_t ASSEMBLYREF_DISABLEJITCOMPILE_OPTIMIZER_FLAG = 0x00004000u;

using Il2CppChar            = uint16_t;
using il2cpp_array_size_t   = uintptr_t;
using TypeDefinitionIndex   = int32_t;
using GenericParameterIndex = int32_t;
using Il2CppNativeChar      = char;

struct Il2CppMemoryCallbacks;
struct Il2CppImage;
struct Il2CppClass;
struct Il2CppArrayBounds;
struct Il2CppAssembly;
struct Il2CppArrayType;
struct Il2CppGenericClass;
struct Il2CppReflectionType;
struct MonitorData;
struct EventInfo;
struct FieldInfo;
struct PropertyInfo;
struct Il2CppDomain;
struct Il2CppException;
struct Il2CppObject;
struct Il2CppReflectionMethod;
struct Il2CppString;
struct Il2CppThread;
struct Il2CppStackFrameInfo;
struct Il2CppManagedMemorySnapshot;
struct Il2CppDebuggerTransport;
struct Il2CppMethodDebugInfo;
struct Il2CppCustomAttrInfo;

using Il2CppVTable                        = Il2CppClass;
using Il2CppMethodPointer                 = void (*)();
using il2cpp_register_object_callback     = void (*)(Il2CppObject** arr, int size, void* userdata);
using il2cpp_liveness_reallocate_callback = void* (*)(void* ptr, size_t size, void* userdata);
using Il2CppFrameWalkFunc                 = void (*)(const Il2CppStackFrameInfo* info, void* user_data);
using Il2CppBacktraceFunc                 = size_t (*)(Il2CppMethodPointer* buffer, size_t maxSize);
using Il2CppSetFindPlugInCallback         = const Il2CppNativeChar* (*)(const Il2CppNativeChar*);
using Il2CppLogCallback                   = void (*)(const char*);

enum class Il2CppRuntimeUnhandledExceptionPolicy : int { Legacy = 0, Current = 1 };
enum class Il2CppGCMode : int { Disabled = 0, Enabled = 1, Manual = 2 };

enum Il2CppStat : int {
    IL2CPP_STAT_NEW_OBJECT_COUNT,
    IL2CPP_STAT_INITIALIZED_CLASS_COUNT,
    IL2CPP_STAT_METHOD_COUNT,
    IL2CPP_STAT_CLASS_STATIC_DATA_SIZE,
    IL2CPP_STAT_GENERIC_INSTANCE_COUNT,
    IL2CPP_STAT_GENERIC_CLASS_COUNT,
    IL2CPP_STAT_INFLATED_METHOD_COUNT,
    IL2CPP_STAT_INFLATED_TYPE_COUNT,
};

enum Il2CppTypeEnum : uint8_t {
    IL2CPP_TYPE_END        = 0x00, IL2CPP_TYPE_VOID        = 0x01,
    IL2CPP_TYPE_BOOLEAN    = 0x02, IL2CPP_TYPE_CHAR        = 0x03,
    IL2CPP_TYPE_I1         = 0x04, IL2CPP_TYPE_U1          = 0x05,
    IL2CPP_TYPE_I2         = 0x06, IL2CPP_TYPE_U2          = 0x07,
    IL2CPP_TYPE_I4         = 0x08, IL2CPP_TYPE_U4          = 0x09,
    IL2CPP_TYPE_I8         = 0x0a, IL2CPP_TYPE_U8          = 0x0b,
    IL2CPP_TYPE_R4         = 0x0c, IL2CPP_TYPE_R8          = 0x0d,
    IL2CPP_TYPE_STRING     = 0x0e, IL2CPP_TYPE_PTR         = 0x0f,
    IL2CPP_TYPE_BYREF      = 0x10, IL2CPP_TYPE_VALUETYPE   = 0x11,
    IL2CPP_TYPE_CLASS      = 0x12, IL2CPP_TYPE_VAR         = 0x13,
    IL2CPP_TYPE_ARRAY      = 0x14, IL2CPP_TYPE_GENERICINST = 0x15,
    IL2CPP_TYPE_TYPEDBYREF = 0x16, IL2CPP_TYPE_I           = 0x18,
    IL2CPP_TYPE_U          = 0x19, IL2CPP_TYPE_FNPTR       = 0x1b,
    IL2CPP_TYPE_OBJECT     = 0x1c, IL2CPP_TYPE_SZARRAY     = 0x1d,
    IL2CPP_TYPE_MVAR       = 0x1e, IL2CPP_TYPE_CMOD_REQD   = 0x1f,
    IL2CPP_TYPE_CMOD_OPT   = 0x20, IL2CPP_TYPE_INTERNAL    = 0x21,
    IL2CPP_TYPE_MODIFIER   = 0x40, IL2CPP_TYPE_SENTINEL    = 0x41,
    IL2CPP_TYPE_PINNED     = 0x45, IL2CPP_TYPE_ENUM        = 0x55,
    IL2CPP_TYPE_IL2CPP_TYPE_INDEX = 0xff
};

struct Il2CppType {
    union {
        void*                 dummy;
        TypeDefinitionIndex   klassIndex;
        const Il2CppType*     type;
        Il2CppArrayType*      array;
        GenericParameterIndex genericParameterIndex;
        Il2CppGenericClass*   generic_class;
    } data;
    unsigned int attrs    : 16;
    Il2CppTypeEnum type   : 8;
    unsigned int num_mods : 6;
    unsigned int byref    : 1;
    unsigned int pinned   : 1;
};

struct MethodInfo {
    Il2CppMethodPointer methodPointer;
};

struct Il2CppObject {
    union { Il2CppClass* klass; Il2CppVTable* vtable; };
    MonitorData* monitor;
};

struct Il2CppArray {
    Il2CppObject        obj;
    Il2CppArrayBounds*  bounds;
    il2cpp_array_size_t max_length;
    void*               vector[32];
};

#define DO_API(r, n, p)           r (*n) p = nullptr;
#define DO_API_NO_RETURN(r, n, p) r (*n) p = nullptr;

DO_API(int,  il2cpp_init,                            (const char* domain_name))
DO_API(int,  il2cpp_init_utf16,                      (const Il2CppChar* domain_name))
DO_API(void, il2cpp_shutdown,                        ())
DO_API(void, il2cpp_set_config_dir,                  (const char* configPath))
DO_API(void, il2cpp_set_data_dir,                    (const char* dataPath))
DO_API(void, il2cpp_set_temp_dir,                    (const char* tempPath))
DO_API(void, il2cpp_set_commandline_arguments,       (int argc, const char* const argv[], const char* basedir))
DO_API(void, il2cpp_set_commandline_arguments_utf16, (int argc, const Il2CppChar* const argv[], const char* basedir))
DO_API(void, il2cpp_set_config_utf16,                (const Il2CppChar* executablePath))
DO_API(void, il2cpp_set_config,                      (const char* executablePath))
DO_API(void, il2cpp_set_memory_callbacks,            (Il2CppMemoryCallbacks* callbacks))
DO_API(const Il2CppImage*,     il2cpp_get_corlib,                    ())
DO_API(void,                   il2cpp_add_internal_call,             (const char* name, Il2CppMethodPointer method))
DO_API(Il2CppMethodPointer,    il2cpp_resolve_icall,                 (const char* name))
DO_API(void*,                  il2cpp_alloc,                         (size_t size))
DO_API(void,                   il2cpp_free,                          (void* ptr))
DO_API(Il2CppClass*,           il2cpp_array_class_get,               (Il2CppClass* element_class, uint32_t rank))
DO_API(uint32_t,               il2cpp_array_length,                  (Il2CppArray* array))
DO_API(uint32_t,               il2cpp_array_get_byte_length,         (Il2CppArray* array))
DO_API(Il2CppArray*,           il2cpp_array_new,                     (Il2CppClass* elementTypeInfo, il2cpp_array_size_t length))
DO_API(Il2CppArray*,           il2cpp_array_new_specific,            (Il2CppClass* arrayTypeInfo, il2cpp_array_size_t length))
DO_API(Il2CppArray*,           il2cpp_array_new_full,                (Il2CppClass* array_class, il2cpp_array_size_t* lengths, il2cpp_array_size_t* lower_bounds))
DO_API(Il2CppClass*,           il2cpp_bounded_array_class_get,       (Il2CppClass* element_class, uint32_t rank, bool bounded))
DO_API(int,                    il2cpp_array_element_size,            (const Il2CppClass* array_class))
DO_API(const Il2CppImage*,     il2cpp_assembly_get_image,            (const Il2CppAssembly* assembly))
DO_API(Il2CppClass*,           il2cpp_class_enum_basetype,           (Il2CppClass* klass))
DO_API(bool,                   il2cpp_class_is_generic,              (const Il2CppClass* klass))
DO_API(bool,                   il2cpp_class_is_inflated,             (const Il2CppClass* klass))
DO_API(bool,                   il2cpp_class_is_assignable_from,      (Il2CppClass* klass, Il2CppClass* oklass))
DO_API(bool,                   il2cpp_class_is_subclass_of,          (Il2CppClass* klass, Il2CppClass* klassc, bool check_interfaces))
DO_API(bool,                   il2cpp_class_has_parent,              (Il2CppClass* klass, Il2CppClass* klassc))
DO_API(Il2CppClass*,           il2cpp_class_from_il2cpp_type,        (const Il2CppType* type))
DO_API(Il2CppClass*,           il2cpp_class_from_name,               (const Il2CppImage* image, const char* namespaze, const char* name))
DO_API(Il2CppClass*,           il2cpp_class_from_system_type,        (Il2CppReflectionType* type))
DO_API(Il2CppClass*,           il2cpp_class_get_element_class,       (Il2CppClass* klass))
DO_API(const EventInfo*,       il2cpp_class_get_events,              (Il2CppClass* klass, void** iter))
DO_API(FieldInfo*,             il2cpp_class_get_fields,              (Il2CppClass* klass, void** iter))
DO_API(Il2CppClass*,           il2cpp_class_get_nested_types,        (Il2CppClass* klass, void** iter))
DO_API(Il2CppClass*,           il2cpp_class_get_interfaces,          (Il2CppClass* klass, void** iter))
DO_API(const PropertyInfo*,    il2cpp_class_get_properties,          (Il2CppClass* klass, void** iter))
DO_API(const PropertyInfo*,    il2cpp_class_get_property_from_name,  (Il2CppClass* klass, const char* name))
DO_API(FieldInfo*,             il2cpp_class_get_field_from_name,     (Il2CppClass* klass, const char* name))
DO_API(const MethodInfo*,      il2cpp_class_get_methods,             (Il2CppClass* klass, void** iter))
DO_API(const MethodInfo*,      il2cpp_class_get_method_from_name,    (const Il2CppClass* klass, const char* name, int argsCount))
DO_API(const char*,            il2cpp_class_get_name,                (Il2CppClass* klass))
DO_API(void,                   il2cpp_type_get_name_chunked,         (const Il2CppType* type, void(*chunkedName)(const char* name, void* userData), void* userData))
DO_API(const char*,            il2cpp_class_get_namespace,           (Il2CppClass* klass))
DO_API(Il2CppClass*,           il2cpp_class_get_parent,              (Il2CppClass* klass))
DO_API(Il2CppClass*,           il2cpp_class_get_declaring_type,      (Il2CppClass* klass))
DO_API(int,                    il2cpp_class_get_rank,                (Il2CppClass* klass))
DO_API(uint32_t,               il2cpp_class_get_flags,               (const Il2CppClass* klass))
DO_API(bool,                   il2cpp_class_is_valuetype,            (const Il2CppClass* klass))
DO_API(bool,                   il2cpp_class_is_enum,                 (const Il2CppClass* klass))
DO_API(bool,                   il2cpp_class_is_abstract,             (const Il2CppClass* klass))
DO_API(bool,                   il2cpp_class_is_interface,            (const Il2CppClass* klass))

#undef DO_API
#undef DO_API_NO_RETURN

namespace entropy {

inline constexpr uint32_t kIl2CppMetadataMagic = 0xFAB11BAFu;

enum class MetadataState : uint8_t { Plain = 0, Encrypted = 1, Unknown = 2 };

struct XorKeyResult {
    std::array<uint8_t, 256> key     = {};
    size_t                   key_len = 0;
    int                      score   = 0;
    bool                     found   = false;
};

[[nodiscard]] static MetadataState analyzeBuffer(
    std::span<const uint8_t> buf) noexcept
{
    if (buf.size() < 4u) return MetadataState::Unknown;
    uint32_t magic = 0;
    __builtin_memcpy(&magic, buf.data(), 4u);
    if (magic == kIl2CppMetadataMagic) return MetadataState::Plain;

    std::array<size_t, 256> freq = {};
    const size_t sample = std::min(buf.size(), size_t{4096u});
    for (size_t i = 0; i < sample; ++i)
        ++freq[buf[i]];

    size_t nonzero = 0;
    for (auto v : freq) if (v > 0) ++nonzero;
    return (nonzero > 200u) ? MetadataState::Encrypted : MetadataState::Unknown;
}

[[nodiscard]] static XorKeyResult discoverXorKey(
    std::span<const uint8_t> buf) noexcept
{
    XorKeyResult result;
    if (buf.size() < 8u) return result;

    constexpr std::array<size_t, 4> kLens = {4u, 8u, 16u, 32u};
    for (size_t key_len : kLens) {
        if (buf.size() < key_len * 2u) continue;

        std::array<uint8_t, 256> candidate = {};
        uint32_t expected = kIl2CppMetadataMagic;
        for (size_t i = 0; i < std::min(key_len, size_t{4u}); ++i)
            candidate[i] = buf[i] ^ static_cast<uint8_t>(expected >> (i * 8u));

        int score = 0;
        for (size_t off = 0; off + key_len <= buf.size(); off += key_len) {
            uint32_t trial = 0;
            for (size_t i = 0; i < std::min(key_len, size_t{4u}); ++i)
                trial |= static_cast<uint32_t>(buf[off + i] ^ candidate[i]) << (i * 8u);
            if (trial == kIl2CppMetadataMagic) ++score;
        }

        if (score > result.score) {
            result.score   = score;
            result.key_len = key_len;
            result.found   = (score >= 2);
            for (size_t i = 0; i < key_len; ++i)
                result.key[i] = candidate[i];
        }
    }
    return result;
}

[[nodiscard]] static std::vector<uint8_t> decryptBuffer(
    std::span<const uint8_t> buf, const XorKeyResult& key)
{
    std::vector<uint8_t> out(buf.size());
    for (size_t i = 0; i < buf.size(); ++i)
        out[i] = buf[i] ^ key.key[i % key.key_len];
    return out;
}

}

namespace async_io {

struct AsyncWriter {
    static constexpr size_t kFlushThreshold = 512u * 1024u;

    [[nodiscard]] bool open(const std::string& path) noexcept {
        fd_ = eaquel::ScopedFd{
            ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644)
        };
        return fd_.valid();
    }

    void write(std::string_view sv) {
        const auto* p = reinterpret_cast<const uint8_t*>(sv.data());
        buf_.insert(buf_.end(), p, p + sv.size());
        if (buf_.size() >= kFlushThreshold) flush();
    }

    void flush() noexcept {
        if (!fd_.valid() || buf_.empty()) return;
        std::span<const uint8_t> remaining{buf_};
        while (!remaining.empty()) {
            const ssize_t w = ::write(fd_.get(), remaining.data(), remaining.size());
            if (w <= 0) break;
            remaining = remaining.subspan(static_cast<size_t>(w));
        }
        buf_.clear();
        fdatasync(fd_.get());
    }

    void close() noexcept {
        flush();
        fd_.reset();
    }

    ~AsyncWriter() noexcept { close(); }

    AsyncWriter()                              = default;
    AsyncWriter(const AsyncWriter&)            = delete;
    AsyncWriter& operator=(const AsyncWriter&) = delete;
    AsyncWriter(AsyncWriter&&)                 = default;
    AsyncWriter& operator=(AsyncWriter&&)      = default;

private:
    eaquel::ScopedFd     fd_;
    std::vector<uint8_t> buf_;
};

}

namespace scanner {

struct MemoryRegion {
    uintptr_t base = 0;
    size_t    size = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return base != 0u && size != 0u;
    }
    [[nodiscard]] constexpr uintptr_t end() const noexcept {
        return base + size;
    }
};

struct ResolvedSymbols {
    uintptr_t domain_get                  = 0;
    uintptr_t domain_get_assemblies       = 0;
    uintptr_t image_get_class             = 0;
    uintptr_t thread_attach               = 0;
    uintptr_t is_vm_thread                = 0;
    uintptr_t domain_get_2026             = 0;
    uintptr_t domain_get_assemblies_2024  = 0;
    uintptr_t image_get_class_2026        = 0;
    uintptr_t metadata_loader             = 0;
    uintptr_t metadata_cache_initialize   = 0;
    uintptr_t metadata_decrypt            = 0;
    uintptr_t il2cpp_init_2026            = 0;
#if defined(__arm__)
    uintptr_t arm32_metadata_loader       = 0;
    uintptr_t arm32_metadata_cache_init   = 0;
    uintptr_t arm32_domain_get            = 0;
    uintptr_t arm32_domain_get_assemblies = 0;
    uintptr_t arm32_image_get_class       = 0;
    uintptr_t arm32_thread_attach         = 0;
#endif
};

namespace known_hashes {
    inline constexpr std::array<uint32_t, 4> kDomainGet64      = {0xA1B2C3D4u, 0xE5F60718u, 0x293A4B5Cu, 0x6D7E8F90u};
    inline constexpr std::array<uint32_t, 4> kMetadataLoader64 = {0x11223344u, 0x55667788u, 0x99AABBCCu, 0xDDEEFF00u};
}

namespace arm64_prologue {
    inline constexpr std::array<uint8_t, 8> kDomainGet                = {0xE0,0x03,0x00,0xAA,0xC0,0x03,0x5F,0xD6};
    inline constexpr std::array<uint8_t, 8> kDomainGet2026            = {0x08,0x00,0x40,0xF9,0xE0,0x03,0x08,0xAA};
    inline constexpr std::array<uint8_t, 8> kDomainGetAssemblies      = {0xE8,0x03,0x00,0xF9,0xE9,0x03,0x01,0xF9};
    inline constexpr std::array<uint8_t, 8> kDomainGetAssemblies2024  = {0x08,0x00,0x40,0xF9,0xE8,0x03,0x01,0xF9};
    inline constexpr std::array<uint8_t, 8> kImageGetClass            = {0xE8,0x03,0x01,0xF9,0xE9,0x03,0x00,0xF9};
    inline constexpr std::array<uint8_t, 8> kImageGetClass2026        = {0x08,0x10,0x40,0xF9,0x28,0x00,0x40,0xF9};
    inline constexpr std::array<uint8_t, 8> kThreadAttach             = {0xE8,0x03,0x40,0xF9,0x08,0x01,0x40,0xF9};
    inline constexpr std::array<uint8_t, 8> kIsVmThread               = {0x08,0x00,0x40,0xF9,0x08,0x01,0x40,0xB9};
    inline constexpr std::array<uint8_t, 8> kMetadataLoader           = {0xFD,0x7B,0xBF,0xA9,0xFD,0x03,0x00,0x91};
    inline constexpr std::array<uint8_t, 8> kMetadataCacheInitialize64= {0xFD,0x7B,0xBF,0xA9,0xF3,0x0F,0x1F,0xF8};
    inline constexpr std::array<uint8_t, 8> kMetadataDecrypt          = {0xFD,0x7B,0xBF,0xA9,0xF3,0x53,0xBF,0xA9};
    inline constexpr std::array<uint8_t, 8> kIl2cppInit2026           = {0xFD,0x7B,0xBF,0xA9,0xF4,0x4F,0xBF,0xA9};
}

#if defined(__arm__)
namespace arm32_prologue {
    inline constexpr std::array<uint8_t, 8> kMetadataLoader32          = {0x2D,0xE9,0xF0,0x47,0x00,0x20,0x90,0xE5};
    inline constexpr std::array<uint8_t, 8> kMetadataCacheInitialize32 = {0x2D,0xE9,0xF8,0x4F,0x05,0x46,0x16,0x46};
    inline constexpr std::array<uint8_t, 8> kDomainGet32               = {0x10,0xB5,0x04,0x46,0x00,0x20,0x90,0xE5};
    inline constexpr std::array<uint8_t, 8> kDomainGetAssemblies32     = {0x2D,0xE9,0xF0,0x41,0x00,0x24,0x90,0xE5};
    inline constexpr std::array<uint8_t, 8> kImageGetClass32           = {0x10,0xB5,0x01,0x24,0x90,0xE5,0x1C,0x00};
    inline constexpr std::array<uint8_t, 8> kThreadAttach32            = {0x2D,0xE9,0x10,0x40,0x01,0x20,0x90,0xE5};
}
#endif

template<size_t N>
[[nodiscard]] static std::optional<uintptr_t> scanPattern(
    const MemoryRegion& region, const std::array<uint8_t, N>& pattern, size_t align = 4u) noexcept
{
    if (!region.valid() || N > region.size) return std::nullopt;
    const std::span<const uint8_t> view{
        reinterpret_cast<const uint8_t*>(region.base), region.size};
    for (size_t i = 0; i + N <= view.size(); i += align) {
        if (__builtin_memcmp(view.data() + i, pattern.data(), N) == 0)
            return region.base + i;
    }
    return std::nullopt;
}

template<size_t N>
[[nodiscard]] static std::optional<uintptr_t> scanPatternFuzzy(
    const MemoryRegion& region, const std::array<uint8_t, N>& pattern, size_t align = 4u) noexcept
{
    static_assert(N >= 2u, "Pattern must have at least 2 bytes");
    if (!region.valid()) return std::nullopt;
    constexpr size_t half = N / 2u;
    const std::span<const uint8_t> view{
        reinterpret_cast<const uint8_t*>(region.base), region.size};
    for (size_t i = 0; i + N <= view.size(); i += align) {
        if (__builtin_memcmp(view.data() + i, pattern.data(), half) == 0)
            return region.base + i;
    }
    return std::nullopt;
}

[[nodiscard]] static std::optional<uintptr_t> scanByHashLattice(
    const MemoryRegion& region, std::span<const uint32_t> hashes) noexcept
{
    if (!region.valid() || hashes.empty()) return std::nullopt;
    if (region.size < sizeof(uint32_t)) return std::nullopt;

    const uintptr_t aligned_base = (region.base + 3u) & ~uintptr_t{3u};
    if (aligned_base >= region.end()) return std::nullopt;
    const size_t count = (region.end() - aligned_base) / sizeof(uint32_t);
    const auto* base_u32 = reinterpret_cast<const uint32_t*>(aligned_base);

    for (size_t i = 0; i < count; ++i) {
        const uint32_t val = base_u32[i];
        for (auto h : hashes) {
            if (val == h) return aligned_base + i * sizeof(uint32_t);
        }
    }
    return std::nullopt;
}

[[nodiscard]] static std::optional<MemoryRegion> resolveExecutableRegion(
    uintptr_t lib_base) noexcept
{
    constexpr uintptr_t kSearchWindow = 0x10000000u;

    eaquel::ScopedFd fd{::open("/proc/self/maps", O_RDONLY | O_CLOEXEC)};
    if (!fd.valid()) return std::nullopt;

    std::string content;
    content.reserve(65536u);
    {
        char chunk[4096];
        ssize_t n = 0;
        while ((n = ::read(fd.get(), chunk, sizeof(chunk))) > 0)
            content.append(chunk, static_cast<size_t>(n));
    }

    std::optional<MemoryRegion> result;
    size_t pos = 0;
    while (pos < content.size()) {
        const size_t eol = content.find('\n', pos);
        const std::string_view line{content.data() + pos,
                                    (eol == std::string::npos ? content.size() : eol) - pos};
        pos = (eol == std::string::npos) ? content.size() : eol + 1u;

        char tmp[512] = {};
        const size_t copy_len = std::min(line.size(), sizeof(tmp) - 1u);
        __builtin_memcpy(tmp, line.data(), copy_len);

        char* end_ptr = nullptr;
        const uintptr_t seg_s = static_cast<uintptr_t>(
            strtoull(tmp, &end_ptr, 16));
        if (!end_ptr || *end_ptr != '-') continue;
        const uintptr_t seg_e = static_cast<uintptr_t>(
            strtoull(end_ptr + 1, &end_ptr, 16));
        if (!end_ptr || *end_ptr != ' ') continue;

        const char* perms = end_ptr + 1;
        if (static_cast<size_t>(perms - tmp) + 4u >= copy_len) continue;
        if (perms[2] != 'x') continue;
        if (seg_s < lib_base || seg_s > lib_base + kSearchWindow) continue;
        if (seg_e <= seg_s) continue;

        result = MemoryRegion{seg_s, seg_e - seg_s};
        break;
    }
    return result;
}

[[nodiscard]] static bool isReadableAddress(uintptr_t address) noexcept {
    if (!address) return false;
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) return false;
    const uintptr_t mask    = static_cast<uintptr_t>(page_size) - 1u;
    const uintptr_t aligned = address & ~mask;

    unsigned char vec = 0;
    return mincore(reinterpret_cast<void*>(aligned),
                   static_cast<size_t>(page_size), &vec) == 0;
}

[[nodiscard]] static ResolvedSymbols scanAllSymbols(uintptr_t library_base) {
    auto region_opt = resolveExecutableRegion(library_base);
    if (!region_opt) {
        LOGE("scanner: region resolve failed base=0x%" PRIxPTR, library_base);
        return {};
    }
    const MemoryRegion& region = *region_opt;
    ResolvedSymbols s;

    const std::vector<uint32_t> dg_hashes(
        known_hashes::kDomainGet64.begin(), known_hashes::kDomainGet64.end());
    const std::vector<uint32_t> ml_hashes(
        known_hashes::kMetadataLoader64.begin(), known_hashes::kMetadataLoader64.end());
    const std::vector<uint32_t> no_hashes;

    auto adaptive_scan = [&](uintptr_t& field,
                              const auto& pat,
                              const std::vector<uint32_t>& hashes,
                              const char* name) {
        if (field) return;
        if (auto a = scanPattern(region, pat))      { field = *a; LOGI("scanner[strict]: %s=0x%" PRIxPTR, name, field); return; }
        if (auto a = scanPatternFuzzy(region, pat)) { field = *a; LOGI("scanner[fuzzy]:  %s=0x%" PRIxPTR, name, field); return; }
        if (!hashes.empty()) {
            if (auto a = scanByHashLattice(region, hashes)) {
                field = *a;
                LOGI("scanner[hash]: %s=0x%" PRIxPTR, name, field);
                return;
            }
        }
        LOGW("scanner: %s not found", name);
    };

    adaptive_scan(s.domain_get,               arm64_prologue::kDomainGet,                dg_hashes, "il2cpp_domain_get");
    adaptive_scan(s.domain_get_assemblies,     arm64_prologue::kDomainGetAssemblies,      no_hashes, "il2cpp_domain_get_assemblies");
    adaptive_scan(s.image_get_class,           arm64_prologue::kImageGetClass,            no_hashes, "il2cpp_image_get_class");
    adaptive_scan(s.thread_attach,             arm64_prologue::kThreadAttach,             no_hashes, "il2cpp_thread_attach");
    adaptive_scan(s.is_vm_thread,              arm64_prologue::kIsVmThread,               no_hashes, "il2cpp_is_vm_thread");
    adaptive_scan(s.domain_get,                arm64_prologue::kDomainGet2026,            dg_hashes, "il2cpp_domain_get[2026]");
    adaptive_scan(s.domain_get_assemblies,     arm64_prologue::kDomainGetAssemblies2024,  no_hashes, "il2cpp_domain_get_assemblies[2024]");
    adaptive_scan(s.image_get_class,           arm64_prologue::kImageGetClass2026,        no_hashes, "il2cpp_image_get_class[2026]");
    adaptive_scan(s.domain_get_2026,           arm64_prologue::kDomainGet2026,            dg_hashes, "domain_get_2026");
    adaptive_scan(s.metadata_loader,           arm64_prologue::kMetadataLoader,           ml_hashes, "metadata_loader");
    adaptive_scan(s.metadata_cache_initialize, arm64_prologue::kMetadataCacheInitialize64, ml_hashes, "metadata_cache_init[64]");
    adaptive_scan(s.metadata_decrypt,          arm64_prologue::kMetadataDecrypt,          no_hashes, "metadata_decrypt");
    adaptive_scan(s.il2cpp_init_2026,          arm64_prologue::kIl2cppInit2026,           no_hashes, "il2cpp_init[2026]");

#if defined(__arm__)
    auto scan32 = [&](uintptr_t& field, const auto& pat, const char* name) {
        if (field) return;
        if (auto a = scanPattern(region, pat, 2u)) {
            field = *a | 1u;
            LOGI("scanner[arm32/strict]: %s=0x%" PRIxPTR, name, field); return;
        }
        if (auto a = scanPatternFuzzy(region, pat, 2u)) {
            field = *a | 1u;
            LOGI("scanner[arm32/fuzzy]: %s=0x%" PRIxPTR, name, field); return;
        }
        LOGW("scanner[arm32]: %s not found", name);
    };
    scan32(s.arm32_metadata_loader,       arm32_prologue::kMetadataLoader32,          "metadata_loader[arm32]");
    scan32(s.arm32_metadata_cache_init,   arm32_prologue::kMetadataCacheInitialize32, "metadata_cache_init[arm32]");
    scan32(s.arm32_domain_get,            arm32_prologue::kDomainGet32,               "domain_get[arm32]");
    scan32(s.arm32_domain_get_assemblies, arm32_prologue::kDomainGetAssemblies32,     "domain_get_assemblies[arm32]");
    scan32(s.arm32_image_get_class,       arm32_prologue::kImageGetClass32,           "image_get_class[arm32]");
    scan32(s.arm32_thread_attach,         arm32_prologue::kThreadAttach32,            "thread_attach[arm32]");
#endif
    return s;
}

[[nodiscard]] static uintptr_t findLibBase(const char* lib_name) noexcept {
    if (!lib_name || !*lib_name) return 0u;

    struct SearchCtx {
        const char* name;
        uintptr_t   result;
    };
    SearchCtx ctx{lib_name, 0u};

    dl_iterate_phdr([](struct dl_phdr_info* info, size_t, void* data) -> int {
        auto* ctx_ = static_cast<SearchCtx*>(data);
        if (info->dlpi_name && strstr(info->dlpi_name, ctx_->name)) {
            ctx_->result = static_cast<uintptr_t>(info->dlpi_addr);
            return 1;
        }
        return 0;
    }, &ctx);

    return ctx.result;
}

static void sigsegv_handler(int sig, siginfo_t* info, void* /*ucontext*/) noexcept {
    constexpr std::string_view msg = "SIGSEGV caught by Eaquel\n";
    ::write(STDERR_FILENO, msg.data(), msg.size());
    (void)sig;
    (void)info;

    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    raise(SIGSEGV);
}

}

namespace process_filter {

[[nodiscard]] static bool isSystemProcess(std::string_view pkg) noexcept {
    constexpr std::array<std::string_view, 12> kSystemPrefixes = {
        "com.android.", "android.", "com.google.android.gms",
        "com.google.android.gsf", "com.sec.", "com.samsung.",
        "com.miui.", "com.huawei.", "com.oppo.", "com.vivo.",
        "com.realme.", "system",
    };
    for (auto prefix : kSystemPrefixes)
        if (pkg.starts_with(prefix)) return true;
    return false;
}

[[nodiscard]] static bool isSystemPackage(std::string_view pkg) noexcept {
    constexpr std::array<std::string_view, 8> kSystemPkgs = {
        "android", "com.android.settings", "com.android.systemui",
        "com.android.phone", "com.android.shell",
        "com.android.inputmethod.latin",
        "com.google.android.webview", "com.android.providers.media",
    };
    for (auto sp : kSystemPkgs)
        if (pkg == sp) return true;
    return false;
}

[[nodiscard]] static bool isThirdPartyApp(std::string_view pkg) noexcept {
    if (pkg.empty() || pkg.size() < 3u) return false;
    if (isSystemProcess(pkg)) return false;
    if (isSystemPackage(pkg)) return false;
    return pkg.find('.') != std::string_view::npos;
}

}

namespace config {

inline constexpr std::string_view kConfigPathPrimary = "/sdcard/Eaquel_Config.json";
inline constexpr std::string_view kConfigPathModule  = "/data/adb/modules/eaquel_dumper/Eaquel_Config.json";
inline constexpr std::string_view kConfigPathKsu     = "/data/adb/ksu/modules/eaquel_dumper/Eaquel_Config.json";
inline constexpr std::string_view kConfigPathApatch  = "/data/adb/apatch/modules/eaquel_dumper/Eaquel_Config.json";
inline constexpr std::string_view kFallbackOutput    = "/sdcard/Eaquel_Dumps";
inline constexpr std::string_view kFallbackOutputMod = "/data/adb/modules/eaquel_dumper/Eaquel_Dumps";
inline constexpr std::string_view kFallbackOutputKsu = "/data/adb/ksu/modules/eaquel_dumper/Eaquel_Dumps";
inline constexpr std::string_view kCacheDir          = "/data/local/tmp/.eaq_cache";

inline constexpr int kAndroidMinApi = 30;
inline constexpr int kAndroidMaxApi = 99;

struct DumperConfig {
    std::string Target_Game;
    std::string Output            = std::string(kFallbackOutput);
    bool        Cpp_Header        = true;
    bool        Frida_Script      = true;
    bool        include_generic   = true;
    bool        Protected_Breaker = true;
    bool        is_64bit          = (sizeof(void*) == 8u);
};

[[nodiscard]] static std::string stripJsonComments(const std::string& src) {
    std::string out;
    out.reserve(src.size());
    size_t start = 0u;
    if (src.size() >= 3u &&
        static_cast<uint8_t>(src[0]) == 0xEFu &&
        static_cast<uint8_t>(src[1]) == 0xBBu &&
        static_cast<uint8_t>(src[2]) == 0xBFu) {
        start = 3u;
    }
    bool in_str = false, escaped = false;
    for (size_t i = start; i < src.size(); ++i) {
        const char c = src[i];
        if (escaped)                     { out += c; escaped = false; continue; }
        if (in_str && c == '\\')         { out += c; escaped = true;  continue; }
        if (c == '"')                    { in_str = !in_str; out += c; continue; }
        if (in_str)                      { out += c; continue; }
        if (c == '/' && i + 1u < src.size() && src[i + 1u] == '/') {
            while (i < src.size() && src[i] != '\n') ++i;
            continue;
        }
        if (c == '#') {
            while (i < src.size() && src[i] != '\n') ++i;
            continue;
        }
        out += c;
    }
    return out;
}

[[nodiscard]] static std::optional<std::string> extractJsonString(
    const std::string& raw_json, std::string_view key)
{
    const std::string json = stripJsonComments(raw_json);
    const std::string needle = std::string("\"").append(key).append("\"");
    size_t search_pos = 0u;
    while (true) {
        const size_t kp = json.find(needle, search_pos);
        if (kp == std::string::npos) return std::nullopt;
        size_t pre = kp;
        while (pre > 0u && (json[pre-1u] == ' ' || json[pre-1u] == '\t' ||
                             json[pre-1u] == '\r' || json[pre-1u] == '\n')) --pre;
        if (pre > 0u && json[pre-1u] != '{' && json[pre-1u] != ',') {
            search_pos = kp + needle.size();
            continue;
        }
        const size_t cp = json.find(':', kp + needle.size());
        if (cp == std::string::npos) return std::nullopt;
        const size_t vs = json.find_first_not_of(" \t\r\n", cp + 1u);
        if (vs == std::string::npos || json[vs] != '"') return std::nullopt;

        std::string result;
        result.reserve(128u);
        size_t i = vs + 1u;
        while (i < json.size() && json[i] != '"') {
            if (json[i] == '\\' && i + 1u < json.size()) {
                ++i;
                switch (json[i]) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'u': {
                        if (i + 4u < json.size()) {
                            char hex[5] = {};
                            __builtin_memcpy(hex, json.data() + i + 1u, 4u);
                            const auto cp2 = static_cast<uint32_t>(
                                strtoul(hex, nullptr, 16));
                            if (cp2 < 0x80u) {
                                result += static_cast<char>(cp2);
                            } else if (cp2 < 0x800u) {
                                result += static_cast<char>(0xC0u | (cp2 >> 6));
                                result += static_cast<char>(0x80u | (cp2 & 0x3Fu));
                            } else {
                                result += static_cast<char>(0xE0u | (cp2 >> 12));
                                result += static_cast<char>(0x80u | ((cp2 >> 6) & 0x3Fu));
                                result += static_cast<char>(0x80u | (cp2 & 0x3Fu));
                            }
                            i += 4u;
                        }
                        break;
                    }
                    default: result += json[i]; break;
                }
            } else {
                result += json[i];
            }
            ++i;
        }
        return result;
    }
}

[[nodiscard]] static bool extractJsonBool(
    const std::string& raw_json, std::string_view key, bool def) noexcept
{
    const std::string json = stripJsonComments(raw_json);
    const std::string needle = std::string("\"").append(key).append("\"");
    const size_t kp = json.find(needle);
    if (kp == std::string::npos) return def;
    const size_t cp = json.find(':', kp + needle.size());
    if (cp == std::string::npos) return def;
    const size_t vs = json.find_first_not_of(" \t\r\n", cp + 1u);
    if (vs == std::string::npos) return def;
    if (json.compare(vs, 4u, "true")  == 0) return true;
    if (json.compare(vs, 5u, "false") == 0) return false;
    if (vs < json.size() && json[vs] == '1') return true;
    if (vs < json.size() && json[vs] == '0') return false;
    return def;
}

[[nodiscard]] static std::string sanitizeString(std::string s) noexcept {
    s.erase(std::remove_if(s.begin(), s.end(),
        [](unsigned char c) { return c < 0x20u || c > 0x7Eu; }), s.end());
    const size_t f = s.find_first_not_of(" \t\r\n");
    if (f == std::string::npos) return {};
    return s.substr(f, s.find_last_not_of(" \t\r\n") - f + 1u);
}

[[nodiscard]] static DumperConfig parseJsonConfig(const std::string& json) {
    DumperConfig cfg;
    cfg.Output   = std::string(kFallbackOutput);
    cfg.is_64bit = (sizeof(void*) == 8u);

    if (auto v = extractJsonString(json, "Target_Game"); v)
        cfg.Target_Game = sanitizeString(std::move(*v));
    if (auto v = extractJsonString(json, "Output"); v && !v->empty())
        cfg.Output = sanitizeString(std::move(*v));

    cfg.Cpp_Header        = extractJsonBool(json, "Cpp_Header",        true);
    cfg.Frida_Script      = extractJsonBool(json, "Frida_Script",      true);
    cfg.include_generic   = extractJsonBool(json, "include_generic",   true);
    cfg.Protected_Breaker = extractJsonBool(json, "Protected_Breaker", true);
    return cfg;
}

[[nodiscard]] static DumperConfig loadConfig() {
    constexpr std::array<std::string_view, 4> paths = {
        kConfigPathPrimary, kConfigPathModule, kConfigPathKsu, kConfigPathApatch
    };
    for (auto path : paths) {
        eaquel::ScopedFd fd{::open(path.data(), O_RDONLY | O_CLOEXEC)};
        if (!fd.valid()) continue;

        std::string json;
        json.reserve(4096u);
        char chunk[4096];
        ssize_t n = 0;
        while ((n = ::read(fd.get(), chunk, sizeof(chunk))) > 0)
            json.append(chunk, static_cast<size_t>(n));

        if (json.empty()) continue;
        auto cfg = parseJsonConfig(json);
        LOGI("config: loaded %s  Target_Game=%s  Output=%s  64bit=%d",
             path.data(), cfg.Target_Game.c_str(), cfg.Output.c_str(),
             static_cast<int>(cfg.is_64bit));
        return cfg;
    }
    LOGW("config: Eaquel_Config.json not found, using defaults");
    DumperConfig cfg;
    cfg.Output = std::string(kFallbackOutput);
    return cfg;
}

[[nodiscard]] static bool isExplicitTarget(const DumperConfig& cfg) noexcept {
    return !cfg.Target_Game.empty() && cfg.Target_Game != "!";
}
[[nodiscard]] static bool isWildcardTarget(const DumperConfig& cfg) noexcept {
    return cfg.Target_Game == "!";
}
[[nodiscard]] static bool isTargetPackage(const DumperConfig& cfg, std::string_view pkg) noexcept {
    if (isWildcardTarget(cfg)) return true;
    return isExplicitTarget(cfg) && cfg.Target_Game == pkg;
}

using HotReloadCallback = std::function<void(const DumperConfig&)>;

class ConfigWatcher {
public:
    ConfigWatcher()                              = default;
    ~ConfigWatcher() noexcept { stop(); }
    ConfigWatcher(const ConfigWatcher&)          = delete;
    ConfigWatcher& operator=(const ConfigWatcher&) = delete;
    ConfigWatcher(ConfigWatcher&&)               = delete;
    ConfigWatcher& operator=(ConfigWatcher&&)    = delete;

    bool start(HotReloadCallback cb) {
        cb_  = std::move(cb);

        ifd_ = eaquel::ScopedFd{inotify_init1(IN_NONBLOCK | IN_CLOEXEC)};
        if (!ifd_.valid()) {
            LOGE("inotify_init1 failed errno=%d", errno);
            return false;
        }
        efd_ = eaquel::ScopedFd{eventfd(0u, EFD_CLOEXEC | EFD_NONBLOCK)};
        if (!efd_.valid()) {
            LOGE("eventfd failed errno=%d", errno);
            ifd_.reset();
            return false;
        }

        constexpr std::array<std::string_view, 4> paths = {
            kConfigPathPrimary, kConfigPathModule, kConfigPathKsu, kConfigPathApatch
        };
        bool any_watch = false;
        for (auto p : paths) {
            std::string dir{p};
            const auto slash = dir.rfind('/');
            if (slash != std::string::npos) dir.resize(slash);
            const int wd = inotify_add_watch(ifd_.get(), dir.c_str(),
                                             IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
            if (wd >= 0) {
                any_watch = true;
                LOGI("inotify: watching %s (wd=%d)", dir.c_str(), wd);
            } else {
                LOGW("inotify: watch failed for %s errno=%d", dir.c_str(), errno);
            }
        }
        if (!any_watch) LOGW("inotify: no dirs watchable — hot-reload disabled");

        running_.store(true, std::memory_order_release);
        thread_ = std::thread([this]() noexcept { watchLoop(); });
        return true;
    }

    void stop() noexcept {
        running_.store(false, std::memory_order_release);
        if (efd_.valid()) {
            const uint64_t v = 1u;
            (void)::write(efd_.get(), &v, sizeof(v));
        }
        if (thread_.joinable()) thread_.join();
        efd_.reset();
        ifd_.reset();
    }

private:
    HotReloadCallback  cb_;
    eaquel::ScopedFd   ifd_;
    eaquel::ScopedFd   efd_;
    std::atomic<bool>  running_{false};
    std::thread        thread_;

    void watchLoop() noexcept {
        constexpr size_t kBufSz = (sizeof(inotify_event) + NAME_MAX + 1u) * 4u;
        std::vector<char> buf(kBufSz);

        struct pollfd fds[2];
        fds[0] = {ifd_.get(), POLLIN, 0};
        fds[1] = {efd_.get(), POLLIN, 0};

        while (running_.load(std::memory_order_acquire)) {
            const int ret = poll(fds, 2, -1);
            if (ret <= 0) continue;
            if (fds[1].revents & POLLIN) break;
            if (!(fds[0].revents & POLLIN)) continue;

            const ssize_t len = ::read(ifd_.get(), buf.data(), buf.size());
            if (len <= 0) continue;

            bool reload = false;
            for (const char* ptr = buf.data(); ptr < buf.data() + len; ) {
                const ptrdiff_t remaining = (buf.data() + len) - ptr;
                if (remaining < static_cast<ptrdiff_t>(sizeof(inotify_event))) break;

                inotify_event ev{};
                __builtin_memcpy(&ev, ptr, sizeof(inotify_event));
                const size_t total = sizeof(inotify_event) + ev.len;

                if (static_cast<ptrdiff_t>(total) > remaining) break;

                if (ev.len == 0u) {
                    reload = true;
                } else {
                    if (strcmp(ptr + sizeof(inotify_event), "Eaquel_Config.json") == 0)
                        reload = true;
                }
                ptr += total;
            }

            if (reload) {
                struct pollfd settle = {ifd_.get(), POLLIN, 0};
                poll(&settle, 1, 80);

                const auto cfg = loadConfig();
                LOGI("config: hot-reload Target_Game=%s Protected_Breaker=%d",
                     cfg.Target_Game.c_str(), static_cast<int>(cfg.Protected_Breaker));
                if (cb_) cb_(cfg);
            }
        }
    }
};

}

#include <jni.h>

#define REZYGISK_API_VERSION 5

namespace rezygisk {

struct Api;
struct AppSpecializeArgs;
struct ServerSpecializeArgs;

struct AppSpecializeArgs {
    jint&          uid;
    jint&          gid;
    jintArray&     gids;
    jint&          runtime_flags;
    jobjectArray&  rlimits;
    jint&          mount_external;
    jstring&       se_info;
    jstring&       nice_name;
    jstring&       instruction_set;
    jstring&       app_data_dir;

    jintArray*     const fds_to_ignore;
    jboolean*      const is_child_zygote;
    jboolean*      const is_top_app;
    jobjectArray*  const pkg_data_info_list;
    jobjectArray*  const whitelisted_data_info_list;
    jboolean*      const mount_data_dirs;
    jboolean*      const mount_storage_dirs;

    AppSpecializeArgs() = delete;
    AppSpecializeArgs(const AppSpecializeArgs&) = delete;
    AppSpecializeArgs& operator=(const AppSpecializeArgs&) = delete;
};

struct ServerSpecializeArgs {
    jint&      uid;
    jint&      gid;
    jintArray& gids;
    jint&      runtime_flags;
    jlong&     permitted_capabilities;
    jlong&     effective_capabilities;

    ServerSpecializeArgs() = delete;
    ServerSpecializeArgs(const ServerSpecializeArgs&) = delete;
    ServerSpecializeArgs& operator=(const ServerSpecializeArgs&) = delete;
};

class ModuleBase {
public:
    virtual void onLoad([[maybe_unused]] Api* api,
                        [[maybe_unused]] JNIEnv* env) noexcept {}
    virtual void preAppSpecialize([[maybe_unused]] AppSpecializeArgs* args) noexcept {}
    virtual void postAppSpecialize([[maybe_unused]] const AppSpecializeArgs* args) noexcept {}
    virtual void preServerSpecialize([[maybe_unused]] ServerSpecializeArgs* args) noexcept {}
    virtual void postServerSpecialize([[maybe_unused]] const ServerSpecializeArgs* args) noexcept {}
    virtual ~ModuleBase() = default;

    ModuleBase()                             = default;
    ModuleBase(const ModuleBase&)            = delete;
    ModuleBase& operator=(const ModuleBase&) = delete;
    ModuleBase(ModuleBase&&)                 = delete;
    ModuleBase& operator=(ModuleBase&&)      = delete;
};

enum class Option : int {
    FORCE_DENYLIST_UNMOUNT = 0,
    DLCLOSE_MODULE_LIBRARY = 1,
};

enum class StateFlag : uint32_t {
    PROCESS_GRANTED_ROOT     = (1u << 0),
    PROCESS_ON_DENYLIST      = (1u << 1),
    PROCESS_IS_SYSTEM_SERVER = (1u << 2),
    PROCESS_IS_ISOLATED      = (1u << 3),
};

[[nodiscard]] constexpr StateFlag operator|(StateFlag a, StateFlag b) noexcept {
    return static_cast<StateFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
[[nodiscard]] constexpr bool hasFlag(uint32_t flags, StateFlag f) noexcept {
    return (flags & static_cast<uint32_t>(f)) != 0u;
}

struct Api {
    [[nodiscard]] int      connectCompanion() noexcept;
    [[nodiscard]] int      getModuleDir() noexcept;
    void                   setOption(Option opt) noexcept;
    [[nodiscard]] uint32_t getFlags() noexcept;
    void                   hookJniNativeMethods(JNIEnv* env, const char* className,
                                                JNINativeMethod* methods, int numMethods) noexcept;
    void                   pltHookRegister(dev_t dev, ino_t inode, const char* symbol,
                                           void* newFunc, void** oldFunc) noexcept;
    void                   pltHookExclude(const char* regex, const char* symbol) noexcept;
    [[nodiscard]] bool     pltHookCommit() noexcept;

private:
    struct api_table* impl = nullptr;
    template <class T> friend void entry_impl_fn(Api*, ModuleBase*, JNIEnv*);
};

namespace internal {

struct api_table {
    void* _this;
    bool (*registerModule)(api_table*, struct module_abi*);

    void (*hookJniNativeMethods)(JNIEnv*, const char*, JNINativeMethod*, int);
    void (*pltHookRegister)(dev_t, ino_t, const char*, void*, void**);
    void (*pltHookExclude)(const char*, const char*);
    bool (*pltHookCommit)();

    int      (*connectCompanion)(void*);
    void     (*setOption)(void*, int);
    int      (*getModuleDir)(void*);
    uint32_t (*getFlags)(void*);
};

struct module_abi {
    int32_t     api_version;
    ModuleBase* _this;

    void (*preAppSpecialize)    (ModuleBase*, AppSpecializeArgs*);
    void (*postAppSpecialize)   (ModuleBase*, const AppSpecializeArgs*);
    void (*preServerSpecialize) (ModuleBase*, ServerSpecializeArgs*);
    void (*postServerSpecialize)(ModuleBase*, const ServerSpecializeArgs*);

    explicit module_abi(ModuleBase* module) noexcept
        : api_version(static_cast<int32_t>(REZYGISK_API_VERSION))
        , _this(module)
    {
        preAppSpecialize     = [](ModuleBase* s, AppSpecializeArgs* a) noexcept          { s->preAppSpecialize(a);      };
        postAppSpecialize    = [](ModuleBase* s, const AppSpecializeArgs* a) noexcept    { s->postAppSpecialize(a);     };
        preServerSpecialize  = [](ModuleBase* s, ServerSpecializeArgs* a) noexcept       { s->preServerSpecialize(a);   };
        postServerSpecialize = [](ModuleBase* s, const ServerSpecializeArgs* a) noexcept { s->postServerSpecialize(a);  };
    }
};

template <class T>
void entry_impl(api_table* table, JNIEnv* env) {
    if (!table) return;

    auto* module = new (std::nothrow) T();
    if (!module) return;

    auto* mod_abi = new (std::nothrow) module_abi(module);
    if (!mod_abi) { delete module; return; }

    if (!table->registerModule(table, mod_abi)) {
        delete mod_abi;
        delete module;
        return;
    }

    auto* api = new (std::nothrow) Api();
    if (!api) { delete mod_abi; delete module; return; }
    api->impl = table;
    module->onLoad(api, env);
}

} // namespace internal

inline int Api::connectCompanion() noexcept {
    return (impl && impl->connectCompanion) ? impl->connectCompanion(impl->_this) : -1;
}
inline int Api::getModuleDir() noexcept {
    return (impl && impl->getModuleDir) ? impl->getModuleDir(impl->_this) : -1;
}
inline void Api::setOption(Option opt) noexcept {
    if (impl && impl->setOption)
        impl->setOption(impl->_this, static_cast<int>(opt));
}
inline uint32_t Api::getFlags() noexcept {
    return (impl && impl->getFlags) ? impl->getFlags(impl->_this) : 0u;
}
inline void Api::hookJniNativeMethods(JNIEnv* env, const char* cls,
                                      JNINativeMethod* m, int n) noexcept {
    if (impl && impl->hookJniNativeMethods)
        impl->hookJniNativeMethods(env, cls, m, n);
}
inline void Api::pltHookRegister(dev_t dev, ino_t ino, const char* sym,
                                  void* nf, void** of) noexcept {
    if (impl && impl->pltHookRegister)
        impl->pltHookRegister(dev, ino, sym, nf, of);
}
inline void Api::pltHookExclude(const char* re, const char* sym) noexcept {
    if (impl && impl->pltHookExclude)
        impl->pltHookExclude(re, sym);
}
inline bool Api::pltHookCommit() noexcept {
    return (impl && impl->pltHookCommit) ? impl->pltHookCommit() : false;
}

#define REGISTER_REZYGISK_MODULE(clazz)                                              \
    namespace rezygisk { namespace internal {                                        \
        void rezygisk_module_entry_impl(::rezygisk::internal::api_table* t,          \
                                        JNIEnv* e) {                                 \
            ::rezygisk::internal::entry_impl<clazz>(t, e);                           \
        }                                                                            \
    }}                                                                               \
    extern "C" [[gnu::visibility("default")]] [[gnu::used]]                          \
    void rezygisk_module_entry(::rezygisk::internal::api_table* t, JNIEnv* e) {      \
        ::rezygisk::internal::entry_impl<clazz>(t, e);                               \
    }                                                                                \
    extern "C" [[gnu::visibility("default")]] [[gnu::used]]                          \
    void zygisk_module_entry(::rezygisk::internal::api_table* t, JNIEnv* e) {        \
        ::rezygisk::internal::entry_impl<clazz>(t, e);                               \
    }

#define REGISTER_ZYGISK_MODULE(clazz) REGISTER_REZYGISK_MODULE(clazz)

#define REGISTER_REZYGISK_COMPANION(func)                                            \
    extern "C" [[gnu::visibility("default")]] [[gnu::used]]                          \
    void rezygisk_companion_entry(int client) { (func)(client); }                    \
    extern "C" [[gnu::visibility("default")]] [[gnu::used]]                          \
    void zygisk_companion_entry(int client) { (func)(client); }

#define REGISTER_ZYGISK_COMPANION(func) REGISTER_REZYGISK_COMPANION(func)

} // namespace rezygisk

namespace zygisk = rezygisk;



namespace zygisk = rezygisk;

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void rezygisk_module_entry(rezygisk::internal::api_table*, JNIEnv*);

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void rezygisk_companion_entry(int);

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void zygisk_module_entry(rezygisk::internal::api_table*, JNIEnv*);

extern "C" [[gnu::visibility("default")]] [[gnu::used]]
void zygisk_companion_entry(int);

#include <dlfcn.h>
#include <cstdlib>

struct ScopedJniString {
    JNIEnv*  env    = nullptr;
    jstring  str    = nullptr;
    const char* raw = nullptr;

    ScopedJniString(JNIEnv* e, jstring s) noexcept : env(e), str(s) {
        if (env && str) raw = env->GetStringUTFChars(str, nullptr);
    }
    ~ScopedJniString() noexcept {
        if (env && str && raw) env->ReleaseStringUTFChars(str, raw);
    }
    [[nodiscard]] std::string owned() const {
        return raw ? std::string{raw} : std::string{};
    }
    [[nodiscard]] bool valid() const noexcept { return raw != nullptr; }

    ScopedJniString(const ScopedJniString&)            = delete;
    ScopedJniString& operator=(const ScopedJniString&) = delete;
};

template<typename T>
struct ScopedLocalRef {
    JNIEnv* env = nullptr;
    T       obj = nullptr;

    ScopedLocalRef(JNIEnv* e, T o) noexcept : env(e), obj(o) {}
    ~ScopedLocalRef() noexcept {
        if (env && obj) env->DeleteLocalRef(obj);
    }
    [[nodiscard]] T get() const noexcept { return obj; }
    [[nodiscard]] bool valid() const noexcept { return obj != nullptr; }

    ScopedLocalRef(const ScopedLocalRef&)            = delete;
    ScopedLocalRef& operator=(const ScopedLocalRef&) = delete;
};

[[nodiscard]] static const char* safeStr(const char* s,
                                          const char* fallback = "") noexcept {
    return (s && *s) ? s : fallback;
}

static void runApiInit(uintptr_t base);
static void runDump(const std::string& out_dir, const config::DumperConfig& cfg);
static void hackStart(const std::string& game_dir, const std::string& out_dir,
                      const config::DumperConfig& cfg);
static void hackPrepare(const std::string& game_dir, const std::string& out_dir,
                        const config::DumperConfig& cfg);
static bool installDlopenHook(const std::string& out_dir,
                               const config::DumperConfig& cfg);
static void installSigsegvHandler() noexcept;

static std::string          s_early_out_dir;
static config::DumperConfig s_early_cfg;
static std::atomic<bool>    s_early_hook_fired{false};
static std::mutex           s_early_cfg_mutex;

[[nodiscard]] static bool isZygoteProcess() noexcept {
    char cmdline[256] = {};
    const eaquel::ScopedFd fd{::open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC)};
    if (!fd.valid()) return true;
    const ssize_t n = ::read(fd.get(), cmdline, sizeof(cmdline) - 1u);
    if (n <= 0) return true;
    const std::string_view sv{cmdline, static_cast<size_t>(n)};
    const std::string_view name{sv.data(), sv.find('\0') == std::string_view::npos
                                            ? sv.size()
                                            : sv.find('\0')};
    return name.starts_with("zygote");
}

[[nodiscard]] static int getAndroidApiLevel() noexcept {
    char prop[PROP_VALUE_MAX] = {};
    __system_property_get("ro.build.version.sdk", prop);
    const int api = static_cast<int>(strtol(prop, nullptr, 10));
    return (api > 0) ? api : 30;
}

[[nodiscard]] static bool isApiLevelSupported() noexcept {
    const int api = getAndroidApiLevel();
    return api >= config::kAndroidMinApi && api <= config::kAndroidMaxApi;
}

[[nodiscard]] static bool isInstalledAsUserApp(std::string_view pkg) noexcept {
    if (pkg.empty()) return false;
    constexpr std::array<std::string_view, 4> kPrefixes = {
        "/data/data/", "/data/user/0/", "/data/user_de/0/", "/data/user/999/"
    };
    for (auto prefix : kPrefixes) {
        std::string path{prefix};
        path.append(pkg);
        struct stat st{};
        if (stat(path.c_str(), &st) == 0 && st.st_uid >= 10000u)
            return true;
    }
    return false;
}

[[nodiscard]] static bool shouldIgnoreProcess(std::string_view pkg) noexcept {
    return pkg.empty()
        || process_filter::isSystemProcess(pkg)
        || process_filter::isSystemPackage(pkg);
}

namespace hook_engine {

[[nodiscard]] static size_t getPageSize() noexcept {
    static const size_t kPage = []() noexcept -> size_t {
        const long ps = sysconf(_SC_PAGESIZE);
        return (ps > 0) ? static_cast<size_t>(ps) : 4096u;
    }();
    return kPage;
}

[[nodiscard]] static uintptr_t alignDown(uintptr_t a, size_t page) noexcept {
    return a & ~(page - 1u);
}
[[nodiscard]] static uintptr_t alignUp(uintptr_t a, size_t page) noexcept {
    return (a + page - 1u) & ~(page - 1u);
}

struct ScopedExecPage {
    void*  rw_map  = MAP_FAILED;
    void*  rx_map  = MAP_FAILED;
    size_t size    = 0;
    int    memfd   = -1;

    [[nodiscard]] bool valid() const noexcept {
        return rx_map != MAP_FAILED && rx_map != nullptr;
    }

    void release() noexcept {
        if (rw_map != MAP_FAILED) { munmap(rw_map, size); rw_map = MAP_FAILED; }
        if (rx_map != MAP_FAILED) { munmap(rx_map, size); rx_map = MAP_FAILED; }
        if (memfd >= 0)           { ::close(memfd); memfd = -1; }
    }

    ~ScopedExecPage() noexcept { release(); }

    ScopedExecPage()                               = default;
    ScopedExecPage(const ScopedExecPage&)          = delete;
    ScopedExecPage& operator=(const ScopedExecPage&) = delete;
    ScopedExecPage(ScopedExecPage&& o) noexcept
        : rw_map(o.rw_map), rx_map(o.rx_map), size(o.size), memfd(o.memfd) {
        o.rw_map = MAP_FAILED; o.rx_map = MAP_FAILED; o.memfd = -1; o.size = 0;
    }
    ScopedExecPage& operator=(ScopedExecPage&& o) noexcept {
        if (this != &o) { release(); rw_map=o.rw_map; rx_map=o.rx_map; size=o.size; memfd=o.memfd;
        o.rw_map=MAP_FAILED; o.rx_map=MAP_FAILED; o.memfd=-1; o.size=0; } return *this;
    }
};

[[nodiscard]] static ScopedExecPage allocTrampolinePage(
    uintptr_t near_addr) noexcept
{
    const size_t page = getPageSize();
    ScopedExecPage result;
    result.size = page;

#if defined(__NR_memfd_create)
    const int mfd = static_cast<int>(syscall(__NR_memfd_create, "eaquel_tramp", 0u));
    if (mfd >= 0) {
        if (ftruncate(mfd, static_cast<off_t>(page)) == 0) {
            void* rw = mmap(nullptr, page, PROT_READ | PROT_WRITE,
                            MAP_SHARED, mfd, 0);
            if (rw != MAP_FAILED) {
                const uintptr_t lo = (near_addr > 0x8000000u) ? (near_addr - 0x8000000u) : 0u;
                const uintptr_t hi = near_addr + 0x8000000u;

                void* rx = MAP_FAILED;

                eaquel::ScopedFd maps_fd{::open("/proc/self/maps", O_RDONLY | O_CLOEXEC)};
                if (maps_fd.valid()) {
                    std::string content;
                    content.reserve(65536u);
                    char chunk[4096];
                    ssize_t n = 0;
                    while ((n = ::read(maps_fd.get(), chunk, sizeof(chunk))) > 0)
                        content.append(chunk, static_cast<size_t>(n));

                    uintptr_t prev_end = lo;
                    size_t pos = 0;
                    while (pos < content.size() && rx == MAP_FAILED) {
                        const size_t eol = content.find('\n', pos);
                        const size_t line_end = (eol == std::string::npos)
                                                ? content.size() : eol;
                        char tmp[512] = {};
                        const size_t copy_len = std::min(line_end - pos, sizeof(tmp) - 1u);
                        __builtin_memcpy(tmp, content.data() + pos, copy_len);
                        pos = (eol == std::string::npos) ? content.size() : eol + 1u;

                        char* ep = nullptr;
                        const uintptr_t seg_s = static_cast<uintptr_t>(strtoull(tmp, &ep, 16));
                        if (!ep || *ep != '-') continue;
                        const uintptr_t seg_e = static_cast<uintptr_t>(strtoull(ep + 1, &ep, 16));

                        if (seg_s > hi) break;
                        if (seg_s > prev_end && (seg_s - prev_end) >= page) {
                            const uintptr_t try_addr = alignUp(prev_end, page);
                            if (try_addr >= lo && try_addr + page <= seg_s
                                && try_addr + page <= hi) {
                                rx = mmap(reinterpret_cast<void*>(try_addr), page,
                                          PROT_READ | PROT_EXEC, MAP_SHARED | MAP_FIXED_NOREPLACE,
                                          mfd, 0);
                                if (rx == MAP_FAILED) rx = MAP_FAILED;
                            }
                        }
                        prev_end = seg_e;
                    }
                }

                if (rx == MAP_FAILED) {
                    rx = mmap(nullptr, page, PROT_READ | PROT_EXEC,
                              MAP_SHARED, mfd, 0);
                }

                if (rx != MAP_FAILED) {
                    result.memfd  = mfd;
                    result.rw_map = rw;
                    result.rx_map = rx;
                    LOGI("hook: dual-map tramp rw=%p rx=%p (memfd)", rw, rx);
                    return result;
                }
                munmap(rw, page);
            }
        }
        ::close(mfd);
    }
#endif

    const uintptr_t lo2 = (near_addr > 0x8000000u) ? (near_addr - 0x8000000u) : 0u;
    const uintptr_t hi2 = near_addr + 0x8000000u;

    eaquel::ScopedFd maps_fd{::open("/proc/self/maps", O_RDONLY | O_CLOEXEC)};
    void* tramp = MAP_FAILED;
    if (maps_fd.valid()) {
        std::string content;
        content.reserve(65536u);
        char chunk[4096];
        ssize_t n = 0;
        while ((n = ::read(maps_fd.get(), chunk, sizeof(chunk))) > 0)
            content.append(chunk, static_cast<size_t>(n));

        uintptr_t prev_end = lo2;
        size_t pos = 0;
        while (pos < content.size() && tramp == MAP_FAILED) {
            const size_t eol = content.find('\n', pos);
            const size_t line_end = (eol == std::string::npos) ? content.size() : eol;
            char tmp[512] = {};
            const size_t copy_len = std::min(line_end - pos, sizeof(tmp) - 1u);
            __builtin_memcpy(tmp, content.data() + pos, copy_len);
            pos = (eol == std::string::npos) ? content.size() : eol + 1u;

            char* ep = nullptr;
            const uintptr_t seg_s = static_cast<uintptr_t>(strtoull(tmp, &ep, 16));
            if (!ep || *ep != '-') continue;
            const uintptr_t seg_e = static_cast<uintptr_t>(strtoull(ep + 1, &ep, 16));

            if (seg_s > hi2) break;
            if (seg_s > prev_end && (seg_s - prev_end) >= page) {
                const uintptr_t try_addr = alignUp(prev_end, page);
                if (try_addr >= lo2 && try_addr + page <= seg_s && try_addr + page <= hi2) {
                    tramp = mmap(reinterpret_cast<void*>(try_addr), page,
                                 PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                                 -1, 0);
                    if (tramp == MAP_FAILED) tramp = MAP_FAILED;
                }
            }
            prev_end = seg_e;
        }
    }

    if (tramp == MAP_FAILED) {
        tramp = mmap(nullptr, page, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    if (tramp == MAP_FAILED || tramp == nullptr) return result;

    result.rw_map = tramp;
    result.rx_map = tramp;
    LOGI("hook: standard mmap tramp=%p (fallback)", tramp);
    return result;
}

static void flushCache(uintptr_t addr, size_t len) noexcept {
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr + len));
}

[[nodiscard]] static bool isInBranchRange(uintptr_t from, uintptr_t to) noexcept {
    const uintptr_t diff = (from > to) ? (from - to) : (to - from);
    return diff <= 0x8000000u;
}

[[nodiscard]] static bool stealthPatchWindow(
    uintptr_t           addr,
    size_t              len,
    const std::function<void(void* rw_ptr)>& patch_fn,
    void*               rw_alias = nullptr)
{
    const size_t page = getPageSize();
    const uintptr_t page_base = alignDown(addr, page);
    const size_t    page_len  = alignUp(len + (addr - page_base), page);

    if (rw_alias) {
        const uintptr_t offset = addr - page_base;
        patch_fn(static_cast<uint8_t*>(rw_alias) + offset);
        flushCache(addr, len);
        return true;
    }

    if (mprotect(reinterpret_cast<void*>(page_base), page_len,
                 PROT_READ | PROT_WRITE) != 0) {
        LOGE("hook: mprotect RW failed addr=0x%" PRIxPTR " errno=%d", addr, errno);
        return false;
    }

    patch_fn(reinterpret_cast<void*>(addr));

    if (mprotect(reinterpret_cast<void*>(page_base), page_len,
                 PROT_READ | PROT_EXEC) != 0) {
        LOGE("hook: mprotect RX failed addr=0x%" PRIxPTR " errno=%d", addr, errno);
        mprotect(reinterpret_cast<void*>(page_base), page_len, PROT_READ | PROT_EXEC);
        return false;
    }
    flushCache(addr, len);
    return true;
}

#if defined(__aarch64__)

static constexpr size_t kHookSize = 16u;

static constexpr uint32_t kLdrX17Imm8  = 0x58000051u;
static constexpr uint32_t kBrX17       = 0xD61F0220u;
static constexpr uint32_t kBtiC        = 0xD503245Fu;
static constexpr uint32_t kNop         = 0xD503201Fu;

struct alignas(8) Trampoline {
    uint32_t bti_c;
    uint8_t  saved[kHookSize];
    uint32_t ldr_x17;
    uint32_t br_x17;
    uint64_t return_addr;
};
static_assert(sizeof(Trampoline) <= 64u, "Trampoline must fit in one cache line group");

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) noexcept {
    const auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) {
        LOGE("hook[arm64]: target 0x%" PRIxPTR " not readable", tgt);
        return false;
    }

    if ((tgt & 3u) != 0u) {
        LOGE("hook[arm64]: target 0x%" PRIxPTR " not 4-byte aligned", tgt);
        return false;
    }

    auto exec_page = allocTrampolinePage(tgt);
    if (!exec_page.valid()) {
        LOGE("hook[arm64]: trampoline alloc failed");
        return false;
    }

    const bool has_dual_map = (exec_page.rw_map != exec_page.rx_map);
    auto* tramp_rw = static_cast<Trampoline*>(exec_page.rw_map);
    auto* tramp_rx = static_cast<Trampoline*>(exec_page.rx_map);

    const uintptr_t tramp_addr = reinterpret_cast<uintptr_t>(tramp_rx);
    if (!isInBranchRange(tgt, tramp_addr)) {
        LOGW("hook[arm64]: trampoline out of ±128MB range — using absolute jump");
    }

    tramp_rw->bti_c       = kBtiC;
    __builtin_memcpy(tramp_rw->saved, reinterpret_cast<const void*>(tgt), kHookSize);
    tramp_rw->ldr_x17     = kLdrX17Imm8;
    tramp_rw->br_x17      = kBrX17;
    tramp_rw->return_addr = static_cast<uint64_t>(tgt + kHookSize);

    if (!has_dual_map) {
        if (mprotect(exec_page.rx_map, exec_page.size, PROT_READ | PROT_EXEC) != 0) {
            LOGE("hook[arm64]: trampoline lock RX failed errno=%d", errno);
            exec_page.release();
            return false;
        }
    }
    flushCache(reinterpret_cast<uintptr_t>(tramp_rx), sizeof(Trampoline));

    alignas(8) uint8_t patch[kHookSize] = {};
    __builtin_memcpy(patch + 0, &kLdrX17Imm8, 4);
    __builtin_memcpy(patch + 4, &kBrX17,      4);
    const uint64_t hook_addr = reinterpret_cast<uint64_t>(hook);
    __builtin_memcpy(patch + 8, &hook_addr, 8);

    const bool ok = stealthPatchWindow(tgt, kHookSize,
        [&patch](void* dst) noexcept {
            __builtin_memcpy(dst, patch, kHookSize);
        });

    if (!ok) {
        exec_page.release();
        return false;
    }

    if (orig_out) {
        *orig_out = reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(tramp_rx) + offsetof(Trampoline, saved));
    }

    exec_page.rw_map = MAP_FAILED;
    exec_page.rx_map = MAP_FAILED;
    exec_page.memfd  = -1;

    LOGI("hook[arm64]: target=%p hook=%p tramp=%p (dual_map=%d)",
         target, hook, static_cast<void*>(tramp_rx), static_cast<int>(has_dual_map));
    return true;
}

#elif defined(__arm__)

static constexpr size_t kHookSize = 8u;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) noexcept {
    const auto tgt_with_thumb = reinterpret_cast<uintptr_t>(target);
    const auto tgt            = tgt_with_thumb & ~uintptr_t{1u};

    if (!scanner::isReadableAddress(tgt)) {
        LOGE("hook[arm32]: target 0x%" PRIxPTR " not readable", tgt);
        return false;
    }

    auto exec_page = allocTrampolinePage(tgt);
    if (!exec_page.valid()) {
        LOGE("hook[arm32]: trampoline alloc failed");
        return false;
    }

    const bool has_dual_map = (exec_page.rw_map != exec_page.rx_map);
    auto* tramp_rw = static_cast<uint8_t*>(exec_page.rw_map);
    auto* tramp_rx = static_cast<uint8_t*>(exec_page.rx_map);

    constexpr uint8_t kLdrPcThumb[] = {0xDF, 0xF8, 0x00, 0xF0};

    __builtin_memcpy(tramp_rw, reinterpret_cast<const void*>(tgt), kHookSize);
    __builtin_memcpy(tramp_rw + kHookSize, kLdrPcThumb, 4u);
    const uint32_t ret_addr = static_cast<uint32_t>(tgt + kHookSize) | 1u;
    __builtin_memcpy(tramp_rw + kHookSize + 4u, &ret_addr, 4u);

    if (!has_dual_map) {
        if (mprotect(exec_page.rx_map, exec_page.size, PROT_READ | PROT_EXEC) != 0) {
            LOGE("hook[arm32]: trampoline lock RX failed errno=%d", errno);
            exec_page.release();
            return false;
        }
    }
    flushCache(reinterpret_cast<uintptr_t>(tramp_rx), kHookSize + 8u);

    const uint32_t hook_thumb = reinterpret_cast<uint32_t>(hook) | 1u;
    alignas(4) uint8_t patch[kHookSize] = {};
    __builtin_memcpy(patch + 0, kLdrPcThumb, 4u);
    __builtin_memcpy(patch + 4, &hook_thumb, 4u);

    const bool ok = stealthPatchWindow(tgt, kHookSize,
        [&patch](void* dst) noexcept {
            __builtin_memcpy(dst, patch, kHookSize);
        });

    if (!ok) {
        exec_page.release();
        return false;
    }

    if (orig_out) {
        *orig_out = reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(tramp_rx) | 1u);
    }

    exec_page.rw_map = MAP_FAILED;
    exec_page.rx_map = MAP_FAILED;
    exec_page.memfd  = -1;

    LOGI("hook[arm32]: target=%p hook=%p tramp=%p (dual_map=%d)",
         target, hook, tramp_rx, static_cast<int>(has_dual_map));
    return true;
}

#elif defined(__x86_64__)

static constexpr size_t kHookSize = 14u;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) noexcept {
    const auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) return false;

    auto exec_page = allocTrampolinePage(tgt);
    if (!exec_page.valid()) { LOGE("hook[x86_64]: trampoline alloc failed"); return false; }

    const bool has_dual_map = (exec_page.rw_map != exec_page.rx_map);
    auto* tramp_rw = static_cast<uint8_t*>(exec_page.rw_map);
    auto* tramp_rx = static_cast<uint8_t*>(exec_page.rx_map);

    __builtin_memcpy(tramp_rw, reinterpret_cast<const void*>(tgt), kHookSize);
    if (!has_dual_map) {
        if (mprotect(exec_page.rx_map, exec_page.size, PROT_READ | PROT_EXEC) != 0) {
            exec_page.release(); return false;
        }
    }
    flushCache(reinterpret_cast<uintptr_t>(tramp_rx), kHookSize);

    alignas(8) uint8_t patch[kHookSize] = {};
    patch[0] = 0xFF; patch[1] = 0x25;
    const uint32_t rip_off = 0u;
    __builtin_memcpy(patch + 2, &rip_off, 4);
    const uint64_t hook_addr = reinterpret_cast<uint64_t>(hook);
    __builtin_memcpy(patch + 6, &hook_addr, 8);

    const bool ok = stealthPatchWindow(tgt, kHookSize,
        [&patch](void* dst) noexcept {
            __builtin_memcpy(dst, patch, kHookSize);
        });

    if (!ok) { exec_page.release(); return false; }
    if (orig_out) *orig_out = tramp_rx;

    exec_page.rw_map = MAP_FAILED;
    exec_page.rx_map = MAP_FAILED;
    exec_page.memfd  = -1;

    LOGI("hook[x86_64]: target=%p hook=%p tramp=%p", target, hook, tramp_rx);
    return true;
}

#elif defined(__i386__)

static constexpr size_t kHookSize = 5u;

[[nodiscard]] bool installInlineHook(void* target, void* hook, void** orig_out) noexcept {
    const auto tgt = reinterpret_cast<uintptr_t>(target);
    if (!scanner::isReadableAddress(tgt)) return false;

    auto exec_page = allocTrampolinePage(tgt);
    if (!exec_page.valid()) { LOGE("hook[x86]: trampoline alloc failed"); return false; }

    const bool has_dual_map = (exec_page.rw_map != exec_page.rx_map);
    auto* tramp_rw = static_cast<uint8_t*>(exec_page.rw_map);
    auto* tramp_rx = static_cast<uint8_t*>(exec_page.rx_map);

    __builtin_memcpy(tramp_rw, reinterpret_cast<const void*>(tgt), kHookSize);
    if (!has_dual_map) {
        if (mprotect(exec_page.rx_map, exec_page.size, PROT_READ | PROT_EXEC) != 0) {
            exec_page.release(); return false;
        }
    }
    flushCache(reinterpret_cast<uintptr_t>(tramp_rx), kHookSize);

    alignas(4) uint8_t patch[kHookSize] = {};
    patch[0] = 0xE9;
    const auto rel = static_cast<uint32_t>(
        reinterpret_cast<uintptr_t>(hook) - tgt - 5u);
    __builtin_memcpy(patch + 1, &rel, 4);

    const bool ok = stealthPatchWindow(tgt, kHookSize,
        [&patch](void* dst) noexcept {
            __builtin_memcpy(dst, patch, kHookSize);
        });

    if (!ok) { exec_page.release(); return false; }
    if (orig_out) *orig_out = tramp_rx;

    exec_page.rw_map = MAP_FAILED;
    exec_page.rx_map = MAP_FAILED;
    exec_page.memfd  = -1;
    return true;
}

#else
[[nodiscard]] bool installInlineHook(void*, void*, void**) noexcept { return false; }
#endif

[[nodiscard]] bool patchGotSlot(void* target_fn, void* hook_fn, void** orig_out) noexcept {
    Dl_info di{};
    if (!dladdr(reinterpret_cast<void*>(&patchGotSlot), &di) || !di.dli_fbase) {
        LOGE("GOT patch: dladdr failed");
        return false;
    }
    const auto self_base = reinterpret_cast<uintptr_t>(di.dli_fbase);
    constexpr uintptr_t kSearchWindow = 0x8000000u;

    eaquel::ScopedFd maps_fd{::open("/proc/self/maps", O_RDONLY | O_CLOEXEC)};
    if (!maps_fd.valid()) return false;

    std::string content;
    content.reserve(65536u);
    char chunk[4096];
    ssize_t n = 0;
    while ((n = ::read(maps_fd.get(), chunk, sizeof(chunk))) > 0)
        content.append(chunk, static_cast<size_t>(n));

    size_t pos = 0;
    while (pos < content.size()) {
        const size_t eol = content.find('\n', pos);
        const size_t line_end = (eol == std::string::npos) ? content.size() : eol;
        char tmp[512] = {};
        const size_t copy_len = std::min(line_end - pos, sizeof(tmp) - 1u);
        __builtin_memcpy(tmp, content.data() + pos, copy_len);
        pos = (eol == std::string::npos) ? content.size() : eol + 1u;

        char* ep = nullptr;
        const uintptr_t seg_s = static_cast<uintptr_t>(strtoull(tmp, &ep, 16));
        if (!ep || *ep != '-') continue;
        const uintptr_t seg_e = static_cast<uintptr_t>(strtoull(ep + 1, &ep, 16));
        if (!ep || *ep != ' ') continue;
        const char* perms = ep + 1;
        if (static_cast<size_t>(perms - tmp) + 4u >= sizeof(tmp)) continue;
        if (perms[1] != 'w') continue;
        if (seg_s < self_base || seg_s > self_base + kSearchWindow) continue;
        if (seg_e <= seg_s) continue;

        const size_t page = getPageSize();
        for (uintptr_t addr = seg_s;
             addr + sizeof(void*) <= seg_e;
             addr += sizeof(void*))
        {
            void* slot_val = nullptr;
            __builtin_memcpy(&slot_val, reinterpret_cast<const void*>(addr), sizeof(void*));
            if (slot_val != target_fn) continue;

            const uintptr_t page_addr = alignDown(addr, page);
            if (mprotect(reinterpret_cast<void*>(page_addr), page,
                         PROT_READ | PROT_WRITE) != 0)
                continue;

            if (orig_out) *orig_out = slot_val;
            __builtin_memcpy(reinterpret_cast<void*>(addr), &hook_fn, sizeof(void*));
            mprotect(reinterpret_cast<void*>(page_addr), page, PROT_READ);
            LOGI("GOT patch: slot=0x%" PRIxPTR " new=%p", addr, hook_fn);
            return true;
        }
    }
    LOGW("GOT patch: slot for %p not found", target_fn);
    return false;
}

}

static void installSigsegvHandler() noexcept {
    struct sigaction sa{};
    sa.sa_sigaction = scanner::sigsegv_handler;
    sa.sa_flags     = static_cast<int>(SA_SIGINFO | SA_RESETHAND);
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
}

namespace metadata_hook {

using MetadataLoadFn = void* (*)(const char* path, size_t* out_size);

static MetadataLoadFn    s_orig_load{nullptr};
static std::atomic<bool> s_fired{false};
static std::string       s_dump_dir;
static std::mutex        s_mutex;

static void persistDecryptedMetadata(const void* data, size_t size,
                                      std::string src_path) noexcept
{
    if (!data || size < 4u) return;

    const std::span<const uint8_t> buf_span{
        reinterpret_cast<const uint8_t*>(data), size};

    const auto state = entropy::analyzeBuffer(buf_span);
    std::vector<uint8_t> plaintext;

    if (state == entropy::MetadataState::Encrypted) {
        LOGI("[MetaHook] encrypted — XOR key scan");
        const auto key = entropy::discoverXorKey(buf_span);
        if (key.found && key.score >= 2) {
            plaintext = entropy::decryptBuffer(buf_span, key);
            LOGI("[MetaHook] decrypt done key_len=%zu score=%d", key.key_len, key.score);
        } else {
            LOGW("[MetaHook] XOR key unverified — writing raw");
            plaintext.assign(buf_span.begin(), buf_span.end());
        }
    } else {
        plaintext.assign(buf_span.begin(), buf_span.end());
    }

    uint32_t out_magic = 0;
    if (plaintext.size() >= 4u)
        __builtin_memcpy(&out_magic, plaintext.data(), 4u);
    if (out_magic != entropy::kIl2CppMetadataMagic)
        LOGW("[MetaHook] output magic 0x%08X — may still be encrypted", out_magic);

    std::string dump_dir_copy;
    {
        std::lock_guard<std::mutex> lk(s_mutex);
        dump_dir_copy = s_dump_dir;
    }

    struct stat st{};
    if (stat(dump_dir_copy.c_str(), &st) != 0)
        mkdir(dump_dir_copy.c_str(), 0755);

    const std::string out_path = dump_dir_copy + "/global-metadata.dat";
    eaquel::ScopedFd fd{::open(out_path.c_str(),
                                O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644)};
    if (!fd.valid()) {
        LOGE("[MetaHook] open(%s) failed errno=%d", out_path.c_str(), errno);
        return;
    }

    std::span<const uint8_t> remaining{plaintext};
    while (!remaining.empty()) {
        const ssize_t w = ::write(fd.get(), remaining.data(), remaining.size());
        if (w <= 0) break;
        remaining = remaining.subspan(static_cast<size_t>(w));
    }
    fdatasync(fd.get());

    LOGI("[MetaHook] metadata written (%zu bytes) %s → %s",
         plaintext.size(), src_path.c_str(), out_path.c_str());
}

static void* hooked_MetadataLoad(const char* path, size_t* out_size) {
    if (!s_orig_load) return nullptr;
    void* result = s_orig_load(path, out_size);
    bool  expected = false;
    if (result && out_size && s_fired.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
    {
        LOGI("[MetaHook] triggered path=%s size=%zu", safeStr(path, "?"), *out_size);
        std::thread([data  = result,
                     size  = *out_size,
                     spath = std::string(path ? path : "")]() mutable noexcept {
        ::setpriority(PRIO_PROCESS, 0, 19);
            persistDecryptedMetadata(data, size, std::move(spath));
        }).detach();
    }
    return result;
}

static bool install(uintptr_t lib_base, const std::string& dump_dir) {
    {
        std::lock_guard<std::mutex> lk(s_mutex);
        s_dump_dir = dump_dir;
    }

    const auto syms   = scanner::scanAllSymbols(lib_base);
    uintptr_t loader_addr = 0;

#if defined(__aarch64__)
    loader_addr = syms.metadata_cache_initialize ? syms.metadata_cache_initialize
                                                  : syms.metadata_loader;
#elif defined(__arm__)
    loader_addr = syms.arm32_metadata_cache_init  ? syms.arm32_metadata_cache_init
                                                   : syms.arm32_metadata_loader;
#elif defined(__x86_64__) || defined(__i386__)
    loader_addr = syms.metadata_cache_initialize ? syms.metadata_cache_initialize
                                                  : syms.metadata_loader;
#endif

    if (!loader_addr) { LOGW("[MetaHook] loader addr not found"); return false; }

    const bool ok = hook_engine::installInlineHook(
        reinterpret_cast<void*>(loader_addr),
        reinterpret_cast<void*>(&hooked_MetadataLoad),
        reinterpret_cast<void**>(&s_orig_load));

    if (ok) LOGI("[MetaHook] hook installed 0x%" PRIxPTR, loader_addr);
    else    LOGE("[MetaHook] hook failed");
    return ok;
}

static void resetForReload(const std::string& new_dump_dir) noexcept {
    std::lock_guard<std::mutex> lk(s_mutex);
    s_dump_dir = new_dump_dir;
    s_fired.store(false, std::memory_order_release);
    LOGI("[MetaHook] hot-reload reset dump_dir=%s", new_dump_dir.c_str());
}

}

static void* (*s_orig_dlopen)(const char*, int) = nullptr;

static void* hooked_dlopen(const char* path, int flags) {
    if (!s_orig_dlopen) return nullptr;
    void* handle = s_orig_dlopen(path, flags);
    if (handle && path && strstr(path, "libil2cpp.so")) {
        bool expected = false;
        if (s_early_hook_fired.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            LOGI("EARLY HOOK: libil2cpp.so loaded path=%s", path);

            std::string out;
            config::DumperConfig cfg;
            {
                std::lock_guard<std::mutex> lk(s_early_cfg_mutex);
                out = s_early_out_dir;
                cfg = s_early_cfg;
            }

            std::thread([out  = std::move(out),
                         cfg  = std::move(cfg),
                         hdl  = handle]() mutable noexcept {
        ::setpriority(PRIO_PROCESS, 0, 19);
                {
                    eaquel::ScopedFd efd{eventfd(0u, EFD_CLOEXEC | EFD_NONBLOCK)};
                    if (efd.valid()) {
                        struct pollfd pf = {efd.get(), POLLIN, 0};
                        poll(&pf, 1, 30);
                    }
                }

                uintptr_t base = 0;
                Dl_info di{};
                if (dladdr(hdl, &di) && di.dli_fbase)
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
        LOGI("dlopen hook: inline installed");
        return true;
    }

    if (hook_engine::patchGotSlot(
            reinterpret_cast<void*>(dlopen),
            reinterpret_cast<void*>(hooked_dlopen),
            reinterpret_cast<void**>(&s_orig_dlopen)))
    {
        s_orig_dlopen = dlopen;
        LOGI("dlopen hook: GOT installed");
        return true;
    }

    LOGW("dlopen hook: failed — polling fallback active");
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
    if (sym) {
        __builtin_memcpy(&out, &sym, sizeof(out));
    } else {
        LOGW("symbol not resolved: %s", name);
    }
}

template<typename T>
static void applyFallback(uintptr_t addr, T& out) noexcept {
    if (!out && addr && scanner::isReadableAddress(addr)) {
        __builtin_memcpy(&out, &addr, sizeof(out));
        LOGI("scanner fallback: 0x%" PRIxPTR, addr);
    }
}

static void populateFunctionTable(uintptr_t base) {
    void* handle = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
    if (!handle) LOGW("dlopen RTLD_NOLOAD failed — pattern scan mode");

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
            if (s) __builtin_memcpy(&g_api.domain_get, &s, sizeof(g_api.domain_get));
        }
        if (!g_api.domain_get_assemblies) {
            void* s = dlsym(handle, "_ZN6il2cpp2vm6Domain12GetAssembliesEPKNS0_8Il2CppDomainEPm");
            if (s) __builtin_memcpy(&g_api.domain_get_assemblies, &s, sizeof(g_api.domain_get_assemblies));
        }
    }

    const bool critical_missing =
        !g_api.domain_get || !g_api.domain_get_assemblies ||
        !g_api.image_get_class || !g_api.thread_attach || !g_api.is_vm_thread;

    if (critical_missing) {
        LOGW("Critical symbols missing — adaptive pattern scan");
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
        if (!g_api.image_get_class)       LOGW("image_get_class missing — reflection fallback");
    }
}

}

[[nodiscard]] static std::string buildMethodModifier(uint32_t flags) {
    std::string out;
    out.reserve(32u);
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

[[nodiscard]] static std::string safeClassName(Il2CppClass* klass) noexcept {
    if (!klass) return "object";
    auto& api = il2cpp_api::g_api;
    const char* name = api.class_get_name ? api.class_get_name(klass) : nullptr;
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
    bool is_abstract  = false;
    bool is_interface = false;
    bool is_enum      = false;
    bool is_valuetype = false;
    bool is_generic   = false;
    std::vector<MethodEntry>   methods;
    std::vector<FieldEntry>    fields;
    std::vector<PropertyEntry> properties;
    std::vector<std::string>   interfaces;
};

[[nodiscard]] static std::string resolveTypeName(const Il2CppType* t) {
    if (!t) return "void";
    auto& api = il2cpp_api::g_api;
    const bool is_byref = api.type_is_byref && api.type_is_byref(t);
    Il2CppClass* klass  = api.class_from_type ? api.class_from_type(t) : nullptr;
    std::string  base   = safeClassName(klass);

    if (klass && api.class_is_generic && api.class_is_generic(klass))        base += "<>";
    else if (klass && api.class_is_inflated && api.class_is_inflated(klass)) base += "<T>";

    if (t->type == IL2CPP_TYPE_SZARRAY || t->type == IL2CPP_TYPE_ARRAY) base += "[]";
    if (is_byref) base = "ref " + base;
    return base;
}

[[nodiscard]] static std::vector<MethodEntry> extractMethods(Il2CppClass* klass) {
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
        e.modifier    = buildMethodModifier(flags);
        const Il2CppType* rt = api.method_get_return_type ? api.method_get_return_type(method) : nullptr;
        e.return_type = resolveTypeName(rt);
        const uint32_t pcount = api.method_get_param_count ? api.method_get_param_count(method) : 0;
        for (uint32_t i = 0; i < pcount; ++i) {
            const Il2CppType* pt = api.method_get_param ? api.method_get_param(method, i) : nullptr;
            const char* pn       = api.method_get_param_name ? api.method_get_param_name(method, i) : nullptr;
            e.params.emplace_back(resolveTypeName(pt), safeStr(pn, "arg"));
        }
        if (method->methodPointer) {
            e.va  = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(method->methodPointer));
            e.rva = (il2cpp_api::g_il2cpp_base > 0) ? (e.va - il2cpp_api::g_il2cpp_base) : e.va;
        }
        result.push_back(std::move(e));
    }
    return result;
}

[[nodiscard]] static std::vector<FieldEntry> extractFields(Il2CppClass* klass) {
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
        e.offset    = api.field_get_offset ? static_cast<uint64_t>(api.field_get_offset(field)) : 0u;
        const uint32_t fg = api.field_get_flags ? api.field_get_flags(field) : 0;
        e.is_static = (fg & FIELD_ATTRIBUTE_STATIC) != 0;
        e.modifier  = e.is_static ? "static " : "";
        result.push_back(std::move(e));
    }
    return result;
}

[[nodiscard]] static std::vector<PropertyEntry> extractProperties(Il2CppClass* klass) {
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
        if (e.has_getter) {
            const MethodInfo* getter = api.property_get_get_method(prop);
            if (getter) {
                const Il2CppType* pt = api.method_get_return_type ? api.method_get_return_type(getter) : nullptr;
                e.type_name = resolveTypeName(pt);
            }
        }
        result.push_back(std::move(e));
    }
    return result;
}

static void ensureDirectory(const std::string& path) noexcept {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0)
        mkdir(path.c_str(), 0755);
}

static void writeCppHeader(const std::string& out_dir,
                           const std::string& image_name,
                           const std::vector<ClassEntry>& classes)
{
    ensureDirectory(out_dir);
    const std::string path = out_dir + "/" + image_name + ".h";
    async_io::AsyncWriter w;
    if (!w.open(path)) { LOGE("header write: %s failed", path.c_str()); return; }

    w.write("#pragma once\n\n");

    for (const auto& cls : classes) {
        std::string block;
        block.reserve(2048u);

        if (!cls.ns.empty()) { block += "namespace "; block += cls.ns; block += " {\n"; }

        if (cls.is_interface)      { block += "struct /* interface */ "; block += cls.name; }
        else if (cls.is_enum)      { block += "enum class "; block += cls.name; }
        else                       { block += "struct "; block += cls.name; }

        if (!cls.parent.empty() && cls.parent != "object") {
            block += " : public "; block += cls.parent;
        }
        block += " {\n";

        for (const auto& fld : cls.fields) {
            char hex[32] = {};
            snprintf(hex, sizeof(hex), "%" PRIx64, fld.offset);
            block += "    "; block += fld.modifier; block += fld.type_name;
            block += " "; block += fld.name; block += "; // 0x"; block += hex; block += "\n";
        }
        for (const auto& m : cls.methods) {
            char rva[32] = {};
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
    LOGI("C++ header: %s (%zu classes)", path.c_str(), classes.size());
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
            char rva[32] = {};
            snprintf(rva, sizeof(rva), "%" PRIx64, m.rva);
            std::string entry;
            entry.reserve(256u);
            entry += "var addr_"; entry += cls.name; entry += "_"; entry += m.name;
            entry += " = il2cpp_base.add(0x"; entry += rva; entry += ");\n";
            entry += "Interceptor.attach(addr_";
            entry += cls.name; entry += "_"; entry += m.name;
            entry += ", {\n    onEnter: function(args) {},\n    onLeave: function(retval) {}\n});\n\n";
            w.write(entry);
        }
    }
    w.close();
    LOGI("Frida script: %s (%zu classes)", path.c_str(), classes.size());
}

static void runApiInit(uintptr_t base) {
    il2cpp_api::g_il2cpp_base = static_cast<uint64_t>(base);
    il2cpp_api::populateFunctionTable(base);
}

static void runDump(const std::string& out_dir, const config::DumperConfig& cfg) {
    auto& api = il2cpp_api::g_api;
    if (!api.domain_get) {
        LOGE("runDump: domain_get null — cancelled");
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

    LOGI("runDump: %zu assemblies", count);
    ensureDirectory(out_dir);

    for (size_t ai = 0; ai < count; ++ai) {
        const Il2CppAssembly* asm_ptr = assemblies[ai];
        if (!asm_ptr) continue;

        const Il2CppImage* image = api.assembly_get_image ? api.assembly_get_image(asm_ptr) : nullptr;
        if (!image) continue;

        const char* img_name = api.image_get_name ? api.image_get_name(image) : nullptr;
        std::string name = safeStr(img_name, "unknown");
        if (name.size() > 4u && name.substr(name.size() - 4u) == ".dll")
            name = name.substr(0u, name.size() - 4u);

        const size_t cls_count = api.image_get_class_count ? api.image_get_class_count(image) : 0u;
        LOGI("image[%zu]: %s (%zu classes)", ai, name.c_str(), cls_count);

        std::vector<ClassEntry> classes;
        classes.reserve(cls_count);

        for (size_t ci = 0; ci < cls_count; ++ci) {
            Il2CppClass* klass = api.image_get_class ? api.image_get_class(image, ci) : nullptr;
            if (!klass) continue;

            ClassEntry e;
            e.name        = safeStr(api.class_get_name      ? api.class_get_name(klass)      : nullptr, "?");
            e.ns          = safeStr(api.class_get_namespace  ? api.class_get_namespace(klass)  : nullptr);
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


[[nodiscard]] static int64_t exponentialDelay(int attempt) noexcept {
    const int clamped = std::min(attempt, 5);
    const int64_t ms = 200LL * (1LL << clamped);
    return std::min(ms, int64_t{2000});
}

static void stealthWait(int64_t ms) noexcept {
    eaquel::ScopedFd efd{eventfd(0u, EFD_CLOEXEC | EFD_NONBLOCK)};
    if (efd.valid()) {
        struct pollfd pf = {efd.get(), POLLIN, 0};
        poll(&pf, 1, static_cast<int>(ms));
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

static void hackStart(const std::string& game_dir, const std::string& out_dir,
                       const config::DumperConfig& cfg)
{
    if (isZygoteProcess()) {
        LOGE("[BootGuard] hackStart in Zygote — abort");
        return;
    }
    if (!isApiLevelSupported()) {
        LOGE("[ApiGuard] API %d out of range [%d-%d] — abort",
             getAndroidApiLevel(), config::kAndroidMinApi, config::kAndroidMaxApi);
        return;
    }

    LOGI("hackStart tid=%d game=%s out=%s api=%d",
         gettid(), game_dir.c_str(), out_dir.c_str(), getAndroidApiLevel());

    constexpr int     kMaxRetries = 30;
    constexpr int64_t kMaxTotalMs = 60'000LL;
    int64_t           elapsed_ms  = 0;

    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        const uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (base) {
            LOGI("libil2cpp.so found base=0x%" PRIxPTR " attempt=%d elapsed=%" PRId64 "ms",
                 base, attempt, elapsed_ms);
            if (cfg.Protected_Breaker && !metadata_hook::s_fired.load(std::memory_order_acquire))
                metadata_hook::install(base, out_dir);
            runApiInit(base);
            runDump(out_dir, cfg);
            return;
        }

        const auto delay = exponentialDelay(attempt);
        elapsed_ms += delay;
        if (elapsed_ms >= kMaxTotalMs) {
            LOGE("[BootGuard] TIMEOUT: libil2cpp.so not found in %" PRId64 "ms", kMaxTotalMs);
            return;
        }
        LOGI("libil2cpp.so not ready attempt=%d/%d delay=%" PRId64 "ms elapsed=%" PRId64 "ms",
             attempt + 1, kMaxRetries, delay, elapsed_ms);
        stealthWait(delay);
    }
    LOGE("libil2cpp.so not found after %d attempts tid=%d", kMaxRetries, gettid());
}

static void hackPrepare(const std::string& game_dir, const std::string& out_dir,
                         const config::DumperConfig& cfg)
{
    LOGI("hack_prepare tid=%d api=%d", gettid(), getAndroidApiLevel());
#if defined(__arm__)
    LOGI("mode: ARMv7/Thumb-2");
#elif defined(__aarch64__)
    LOGI("mode: AArch64 (BTI/PAC aware)");
#elif defined(__x86_64__)
    LOGI("mode: x86_64");
#elif defined(__i386__)
    LOGI("mode: x86");
#endif
    hackStart(game_dir, out_dir, cfg);
}


class EaquelDumperModule final : public rezygisk::ModuleBase {
public:
    void onLoad(rezygisk::Api* api_in, JNIEnv* env_in) noexcept override {
        if (!api_in) {
            LOGE("[Module] onLoad: api NULL — module cannot function");
            return;
        }
        api_ = api_in;
        env_ = env_in;
        LOGI("[Module] onLoad: registered — waiting for preAppSpecialize");
    }

    void preAppSpecialize(rezygisk::AppSpecializeArgs* args) noexcept override {
        if (!api_) {
            LOGE("[Module] preAppSpecialize: api NULL — skip");
            return;
        }
        if (!args) {
            LOGI("[Module] preAppSpecialize: args NULL — unloading");
            api_->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const ScopedJniString j_nice (env_, args->nice_name);
        const ScopedJniString j_dir  (env_, args->app_data_dir);
        const ScopedJniString j_iset (env_, args->instruction_set);

        const std::string nice_str = j_nice.owned();
        const std::string dir_str  = j_dir.owned();
        const std::string iset_str = j_iset.owned();

        auto sanitizePkg = [](std::string s) -> std::string {
            return config::sanitizeString(std::move(s));
        };

        auto isValidPkg = [](const std::string& s) noexcept -> bool {
            if (s.size() < 3u) return false;
            if (s.find('.') == std::string::npos) return false;
            if (s.find(':') != std::string::npos) return false;
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
            const std::string cand = sanitizePkg(dir.substr(slash + 1u));
            return isValidPkg(cand) ? cand : std::string{};
        };

        std::string pkg_str = extractPkgFromDir(dir_str);

        if (pkg_str.empty() && args->pkg_data_info_list && *args->pkg_data_info_list) {
            const jsize list_len = env_->GetArrayLength(*args->pkg_data_info_list);
            if (list_len > 0) {
                auto* jelem = static_cast<jstring>(
                    env_->GetObjectArrayElement(*args->pkg_data_info_list, 0));
                ScopedLocalRef<jstring> elem_ref{env_, jelem};
                if (elem_ref.valid()) {
                    const ScopedJniString js_elem{env_, elem_ref.get()};
                    const std::string cand = sanitizePkg(js_elem.owned());
                    if (isValidPkg(cand)) {
                        pkg_str = cand;
                        LOGI("[Module] pkg from pkg_data_info_list='%s'", pkg_str.c_str());
                    }
                }
            }
        }

        if (pkg_str.empty()) {
            const std::string cand = sanitizePkg(nice_str);
            if (isValidPkg(cand)) {
                pkg_str = cand;
                LOGI("[Module] pkg from nice_name='%s'", pkg_str.c_str());
            }
        }

        if (pkg_str.empty() && args->uid >= 10000) {
            const int target_uid = static_cast<int>(args->uid);
            eaquel::ScopedFd plist_fd{::open("/data/system/packages.list", O_RDONLY | O_CLOEXEC)};
            if (plist_fd.valid()) {
                char buf[65536] = {};
                ssize_t n = ::read(plist_fd.get(), buf, sizeof(buf) - 1u);
                if (n > 0) {
                    std::string_view content{buf, static_cast<size_t>(n)};
                    size_t pos = 0;
                    while (pos < content.size()) {
                        const size_t eol = content.find('\n', pos);
                        const std::string_view line = content.substr(pos,
                            eol == std::string_view::npos ? content.size() - pos : eol - pos);
                        pos = (eol == std::string_view::npos) ? content.size() : eol + 1u;

                        char pname[256] = {};
                        int  puid = 0;
                        char tmp[512] = {};
                        const size_t copy_len = std::min(line.size(), sizeof(tmp) - 1u);
                        __builtin_memcpy(tmp, line.data(), copy_len);
                        if (sscanf(tmp, "%255s %d", pname, &puid) == 2 && puid == target_uid) {
                            const std::string cand = sanitizePkg(pname);
                            if (isValidPkg(cand)) {
                                pkg_str = cand;
                                LOGI("[Module] pkg from packages.list uid=%d → '%s'",
                                     target_uid, pkg_str.c_str());
                                break;
                            }
                        }
                    }
                }
            }
        }

        LOGI("[Module] preAppSpecialize: pkg='%s' iset='%s' uid=%d api=%d",
             pkg_str.empty() ? "<empty>" : pkg_str.c_str(),
             iset_str.empty() ? "<empty>" : iset_str.c_str(),
             static_cast<int>(args->uid), getAndroidApiLevel());

        if (pkg_str.empty()) {
            LOGW("[Module] empty package — unloading");
            api_->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        if (shouldIgnoreProcess(pkg_str)) {
            LOGI("[Module] system/ignored pkg='%s' — unloading", pkg_str.c_str());
            api_->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const config::DumperConfig fresh_cfg = config::loadConfig();
        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            active_cfg_ = fresh_cfg;
        }

        if (config::isExplicitTarget(fresh_cfg)) {
            const std::string tgt_clean = config::sanitizeString(fresh_cfg.Target_Game);
            const bool matched = (!tgt_clean.empty() && tgt_clean == pkg_str);

            LOGI("[Module] explicit check tgt='%s' pkg='%s' matched=%s",
                 tgt_clean.c_str(), pkg_str.c_str(), matched ? "YES" : "NO");

            if (!matched) {
                LOGI("[Module] no match — unloading");
                api_->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
                return;
            }
            LOGI("[Module] MATCHED explicit target='%s'", pkg_str.c_str());
            prepareTarget(pkg_str, dir_str, fresh_cfg);
            return;
        }

        if (config::isWildcardTarget(fresh_cfg)) {
            if (!process_filter::isThirdPartyApp(pkg_str) && !isInstalledAsUserApp(pkg_str)) {
                LOGI("[Module] wildcard but '%s' not user app — unloading", pkg_str.c_str());
                api_->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
                return;
            }
            LOGI("[Module] MATCHED wildcard '%s'", pkg_str.c_str());
            prepareTarget(pkg_str, dir_str, fresh_cfg);
            return;
        }

        LOGW("[Module] no valid target for '%s' — unloading", pkg_str.c_str());
        api_->setOption(rezygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

    void postAppSpecialize(const rezygisk::AppSpecializeArgs*) noexcept override {
        if (!enable_hack_) return;
        if (!api_) {
            LOGE("[Module] postAppSpecialize: api NULL — abort");
            return;
        }
        if (isZygoteProcess()) {
            LOGE("[BootGuard] postAppSpecialize in Zygote — cancelled");
            enable_hack_ = false;
            return;
        }
        if (!isApiLevelSupported()) {
            LOGE("[ApiGuard] API %d not supported — cancelled", getAndroidApiLevel());
            enable_hack_ = false;
            return;
        }

        LOGI("[Module] postAppSpecialize: confirmed, starting operations");
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

        if (cfg_copy.Protected_Breaker)
            installDlopenHook(out_copy, cfg_copy);

        startConfigWatcher();

        if (s_orig_dlopen && s_orig_dlopen != dlopen) {
            LOGI("postAppSpecialize: dlopen hook active — waiting for trigger");
            return;
        }

        std::thread([g = std::move(game_copy),
                     o = std::move(out_copy),
                     c = std::move(cfg_copy)]() mutable noexcept {
        ::setpriority(PRIO_PROCESS, 0, 19);
            hackPrepare(g, o, c);
        }).detach();
    }

private:
    rezygisk::Api*       api_         = nullptr;
    JNIEnv*              env_         = nullptr;
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
        LOGI("prepareTarget: pkg=%s dir=%s api=%d",
             pkg.c_str(), app_data_dir.c_str(), getAndroidApiLevel());
        enable_hack_ = true;
        {
            std::lock_guard<std::mutex> lk(cfg_mutex_);
            game_dir_   = app_data_dir;
            out_dir_    = cfg.Output.empty()
                          ? std::string(config::kFallbackOutput) : cfg.Output;
            active_cfg_ = cfg;
        }
        LOGI("TARGET CONFIRMED: %s [64bit=%d api=%d out=%s]",
             pkg.c_str(), static_cast<int>(cfg.is_64bit),
             getAndroidApiLevel(), out_dir_.c_str());
    }

    void triggerReDump(const config::DumperConfig& new_cfg) {
        bool expected = false;
        if (!dump_running_.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel, std::memory_order_acquire)) {
            LOGI("hot-reload: dump already running, skipping");
            return;
        }

        const std::string new_out = new_cfg.Output.empty()
                                    ? std::string(config::kFallbackOutput)
                                    : new_cfg.Output;

        if (new_cfg.Protected_Breaker) metadata_hook::resetForReload(new_out);

        const uintptr_t base = scanner::findLibBase("libil2cpp.so");
        if (!base) {
            std::string g;
            { std::lock_guard<std::mutex> lk(cfg_mutex_); g = game_dir_; }
            config::DumperConfig c = new_cfg;
            std::thread([g = std::move(g), o = new_out, c = std::move(c), this]() mutable noexcept {
        ::setpriority(PRIO_PROCESS, 0, 19);
                hackPrepare(g, o, c);
                dump_running_.store(false, std::memory_order_release);
            }).detach();
            return;
        }

        config::DumperConfig c = new_cfg;
        std::thread([base, o = new_out, c = std::move(c), this]() mutable noexcept {
        ::setpriority(PRIO_PROCESS, 0, 19);
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
            LOGI("hot-reload: Target=%s Cpp=%d Frida=%d Out=%s",
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
        LOGI("[BootGuard] JNI_OnLoad: Zygote — safe return");
        return JNI_VERSION_1_6;
    }
    if (!isApiLevelSupported()) {
        LOGI("[ApiGuard] JNI_OnLoad: API %d not in [%d-%d] — safe return",
             getAndroidApiLevel(), config::kAndroidMinApi, config::kAndroidMaxApi);
        return JNI_VERSION_1_6;
    }

    LOGI("[JNI_OnLoad] tid=%d", gettid());
    installSigsegvHandler();

    config::DumperConfig cfg = config::loadConfig();
    std::string out = cfg.Output.empty()
                      ? std::string(config::kFallbackOutput)
                      : cfg.Output;
    std::string game;

    if (reserved) {
        const std::string combined{static_cast<const char*>(reserved)};
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
                 c = std::move(cfg)]() mutable noexcept {
    ::setpriority(PRIO_PROCESS, 0, 19);
        hackStart(g, o, c);
    }).detach();

    return JNI_VERSION_1_6;
}
#endif
