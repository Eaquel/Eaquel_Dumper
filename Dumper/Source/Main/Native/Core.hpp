#pragma once

#include <android/log.h>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <cmath>
#include <optional>
#include <array>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <span>
#include <ranges>
#include <concepts>
#include <expected>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/eventfd.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <link.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>

#define LOG_TAG "Eaquel"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

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
enum class Il2CppGCMode : int                          { Disabled = 0, Enabled = 1, Manual = 2 };

enum Il2CppStat {
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

#if defined(DO_API)

DO_API(int,  il2cpp_init,                          (const char* domain_name))
DO_API(int,  il2cpp_init_utf16,                    (const Il2CppChar* domain_name))
DO_API(void, il2cpp_shutdown,                      ())
DO_API(void, il2cpp_set_config_dir,                (const char *configPath))
DO_API(void, il2cpp_set_data_dir,                  (const char *dataPath))
DO_API(void, il2cpp_set_temp_dir,                  (const char *tempPath))
DO_API(void, il2cpp_set_commandline_arguments,     (int argc, const char* const argv[], const char* basedir))
DO_API(void, il2cpp_set_commandline_arguments_utf16, (int argc, const Il2CppChar* const argv[], const char* basedir))
DO_API(void, il2cpp_set_config_utf16,              (const Il2CppChar * executablePath))
DO_API(void, il2cpp_set_config,                    (const char* executablePath))
DO_API(void, il2cpp_set_memory_callbacks,          (Il2CppMemoryCallbacks *callbacks))
DO_API(const Il2CppImage*, il2cpp_get_corlib,      ())
DO_API(void, il2cpp_add_internal_call,             (const char* name, Il2CppMethodPointer method))
DO_API(Il2CppMethodPointer, il2cpp_resolve_icall,  (const char* name))
DO_API(void*, il2cpp_alloc,                        (size_t size))
DO_API(void,  il2cpp_free,                         (void* ptr))
DO_API(Il2CppClass*, il2cpp_array_class_get,       (Il2CppClass* element_class, uint32_t rank))
DO_API(uint32_t,     il2cpp_array_length,          (Il2CppArray* array))
DO_API(uint32_t,     il2cpp_array_get_byte_length, (Il2CppArray* array))
DO_API(Il2CppArray*, il2cpp_array_new,             (Il2CppClass* elementTypeInfo, il2cpp_array_size_t length))
DO_API(Il2CppArray*, il2cpp_array_new_specific,    (Il2CppClass* arrayTypeInfo, il2cpp_array_size_t length))
DO_API(Il2CppArray*, il2cpp_array_new_full,        (Il2CppClass* array_class, il2cpp_array_size_t *lengths, il2cpp_array_size_t *lower_bounds))
DO_API(Il2CppClass*, il2cpp_bounded_array_class_get, (Il2CppClass* element_class, uint32_t rank, bool bounded))
DO_API(int,          il2cpp_array_element_size,    (const Il2CppClass* array_class))
DO_API(const Il2CppImage*, il2cpp_assembly_get_image, (const Il2CppAssembly* assembly))
DO_API(Il2CppClass*, il2cpp_class_enum_basetype,   (Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_generic,      (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_inflated,     (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_assignable_from, (Il2CppClass* klass, Il2CppClass* oklass))
DO_API(bool,         il2cpp_class_is_subclass_of,  (Il2CppClass* klass, Il2CppClass* klassc, bool check_interfaces))
DO_API(bool,         il2cpp_class_has_parent,      (Il2CppClass* klass, Il2CppClass* klassc))
DO_API(Il2CppClass*, il2cpp_class_from_il2cpp_type,(const Il2CppType* type))
DO_API(Il2CppClass*, il2cpp_class_from_name,       (const Il2CppImage* image, const char* namespaze, const char *name))
DO_API(Il2CppClass*, il2cpp_class_from_system_type,(Il2CppReflectionType *type))
DO_API(Il2CppClass*, il2cpp_class_get_element_class, (Il2CppClass* klass))
DO_API(const EventInfo*, il2cpp_class_get_events,  (Il2CppClass* klass, void* *iter))
DO_API(FieldInfo*,   il2cpp_class_get_fields,      (Il2CppClass* klass, void* *iter))
DO_API(Il2CppClass*, il2cpp_class_get_nested_types,(Il2CppClass* klass, void* *iter))
DO_API(Il2CppClass*, il2cpp_class_get_interfaces,  (Il2CppClass* klass, void* *iter))
DO_API(const PropertyInfo*, il2cpp_class_get_properties, (Il2CppClass* klass, void* *iter))
DO_API(const PropertyInfo*, il2cpp_class_get_property_from_name, (Il2CppClass* klass, const char *name))
DO_API(FieldInfo*,   il2cpp_class_get_field_from_name, (Il2CppClass* klass, const char *name))
DO_API(const MethodInfo*, il2cpp_class_get_methods,(Il2CppClass* klass, void* *iter))
DO_API(const MethodInfo*, il2cpp_class_get_method_from_name, (const Il2CppClass* klass, const char* name, int argsCount))
DO_API(const char*,  il2cpp_class_get_name,        (Il2CppClass* klass))
DO_API(void,         il2cpp_type_get_name_chunked,  (const Il2CppType* type, void(*chunkedName)(const char* name, void* userData), void* userData))
DO_API(const char*,  il2cpp_class_get_namespace,   (Il2CppClass* klass))
DO_API(Il2CppClass*, il2cpp_class_get_parent,      (Il2CppClass* klass))
DO_API(Il2CppClass*, il2cpp_class_get_declaring_type, (Il2CppClass* klass))
DO_API(int,          il2cpp_class_get_rank,        (Il2CppClass* klass))
DO_API(uint32_t,     il2cpp_class_get_flags,       (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_valuetype,    (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_enum,         (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_abstract,     (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_interface,    (const Il2CppClass* klass))
DO_API(const char*,  il2cpp_class_get_image_name,  (const Il2CppClass* klass))
DO_API(size_t,       il2cpp_image_get_class_count, (const Il2CppImage* image))
DO_API(Il2CppClass*, il2cpp_image_get_class,       (const Il2CppImage* image, size_t index))
DO_API(const char*,  il2cpp_image_get_name,        (const Il2CppImage* image))
DO_API(Il2CppDomain*, il2cpp_domain_get,           ())
DO_API(const Il2CppAssembly**, il2cpp_domain_get_assemblies, (const Il2CppDomain* domain, size_t* size))
DO_API(Il2CppThread*, il2cpp_thread_attach,        (Il2CppDomain* domain))
DO_API(bool,          il2cpp_is_vm_thread,         (Il2CppThread* thread))
DO_API(Il2CppString*, il2cpp_string_new,           (const char* str))
DO_API(const Il2CppType*, il2cpp_class_get_type,   (Il2CppClass* klass))
DO_API(Il2CppClass*, il2cpp_class_from_type,       (const Il2CppType* type))
DO_API(bool,         il2cpp_type_is_byref,         (const Il2CppType* type))
DO_API(const char*,  il2cpp_field_get_name,        (FieldInfo* field))
DO_API(uint32_t,     il2cpp_field_get_flags,       (FieldInfo* field))
DO_API(size_t,       il2cpp_field_get_offset,      (FieldInfo* field))
DO_API(const Il2CppType*, il2cpp_field_get_type,   (FieldInfo* field))
DO_API(void,         il2cpp_field_static_get_value,(FieldInfo* field, void* value))
DO_API(const char*,  il2cpp_property_get_name,     (PropertyInfo* prop))
DO_API(uint32_t,     il2cpp_property_get_flags,    (PropertyInfo* prop))
DO_API(const MethodInfo*, il2cpp_property_get_get_method, (PropertyInfo* prop))
DO_API(const MethodInfo*, il2cpp_property_get_set_method, (PropertyInfo* prop))
DO_API(const char*,  il2cpp_method_get_name,       (const MethodInfo* method))
DO_API(uint32_t,     il2cpp_method_get_flags,      (const MethodInfo* method, uint32_t* iflags))
DO_API(uint32_t,     il2cpp_method_get_param_count,(const MethodInfo* method))
DO_API(const Il2CppType*, il2cpp_method_get_param, (const MethodInfo* method, uint32_t index))
DO_API(const Il2CppType*, il2cpp_method_get_return_type, (const MethodInfo* method))
DO_API(const char*,  il2cpp_method_get_param_name, (const MethodInfo* method, uint32_t index))

#endif

namespace process_filter {

inline constexpr std::string_view kSystemPrefixes[] = {
    "com.android.", "android.", "com.google.android.",
    "com.qualcomm.", "com.mediatek.", "com.samsung.android.",
    "com.huawei.android.", "com.miui.", "com.sec.android.",
    "com.lge.", "com.htc.", "com.sony.", "com.oneplus.",
    "com.oppo.", "com.vivo.", "com.realme.",
    "system", "com.android.systemui", "com.android.settings",
};

inline constexpr std::string_view kSystemPackages[] = {
    "android", "com.android.phone", "com.android.launcher",
    "com.android.launcher2", "com.android.launcher3",
    "com.google.android.gms", "com.google.android.gsf",
    "com.android.inputmethod.latin", "com.android.bluetooth",
    "com.android.nfc", "com.android.wifi", "com.android.location",
    "com.android.camera", "com.android.camera2",
};

[[nodiscard]] static bool isSystemProcess(std::string_view pkg) noexcept {
    for (auto& prefix : kSystemPrefixes) {
        if (pkg.starts_with(prefix)) return true;
    }
    return false;
}

[[nodiscard]] static bool isSystemPackage(std::string_view pkg) noexcept {
    for (auto& sp : kSystemPackages) {
        if (pkg == sp) return true;
    }
    return false;
}

[[nodiscard]] static bool isThirdPartyApp(std::string_view pkg) noexcept {
    if (pkg.empty()) return false;
    if (isSystemProcess(pkg)) return false;
    if (isSystemPackage(pkg)) return false;
    return pkg.find('.') != std::string_view::npos;
}

} // namespace process_filter

namespace entropy {

inline constexpr uint32_t kIl2CppMetadataMagic = 0xFAB11BAF;

enum class MetadataState { Valid, Encrypted, Unknown };

struct XorKeyResult {
    std::vector<uint8_t> key;
    size_t               key_len = 0;
    int                  score   = 0;
    bool                 found   = false;
};

[[nodiscard]] static MetadataState analyzeBuffer(const uint8_t* buf, size_t sz) noexcept {
    if (!buf || sz < 4) return MetadataState::Unknown;
    uint32_t magic = 0;
    memcpy(&magic, buf, 4);
    if (magic == kIl2CppMetadataMagic) return MetadataState::Valid;
    return MetadataState::Encrypted;
}

[[nodiscard]] static XorKeyResult discoverXorKey(const uint8_t* buf, size_t sz) {
    XorKeyResult result;
    if (!buf || sz < 16) return result;

    for (size_t key_len : {4u, 8u, 16u, 32u}) {
        if (sz < key_len * 4) continue;
        std::vector<uint8_t> candidate(key_len);
        uint32_t target = kIl2CppMetadataMagic;
        for (size_t i = 0; i < 4 && i < key_len; ++i)
            candidate[i] = buf[i] ^ ((target >> (i * 8)) & 0xFFu);
        for (size_t i = key_len; i < std::min(sz, key_len * 4); ++i)
            if ((buf[i] ^ candidate[i % key_len]) != 0x00)
                goto next_key_len;
        {
            int score = 0;
            for (size_t j = 4; j < std::min(sz, size_t(256)); ++j) {
                uint8_t decoded = buf[j] ^ candidate[j % key_len];
                if (decoded >= 0x20 && decoded <= 0x7E) ++score;
            }
            if (score > result.score) {
                result.key     = candidate;
                result.key_len = key_len;
                result.score   = score;
                result.found   = true;
            }
        }
        next_key_len:;
    }
    return result;
}

[[nodiscard]] static std::vector<uint8_t> decryptBuffer(
    const uint8_t* buf, size_t sz, const XorKeyResult& key)
{
    std::vector<uint8_t> out(sz);
    for (size_t i = 0; i < sz; ++i)
        out[i] = buf[i] ^ key.key[i % key.key_len];
    return out;
}

} // namespace entropy

namespace scanner {

struct MemoryRegion {
    uintptr_t base;
    size_t    size;
};

struct ResolvedSymbols {
    uintptr_t domain_get               = 0;
    uintptr_t domain_get_assemblies    = 0;
    uintptr_t image_get_class          = 0;
    uintptr_t thread_attach            = 0;
    uintptr_t is_vm_thread             = 0;
    uintptr_t domain_get_2026          = 0;
    uintptr_t domain_get_assemblies_2024 = 0;
    uintptr_t image_get_class_2026     = 0;
    uintptr_t metadata_loader          = 0;
    uintptr_t metadata_cache_initialize= 0;
    uintptr_t metadata_decrypt         = 0;
    uintptr_t il2cpp_init_2026         = 0;
#if defined(__arm__)
    uintptr_t arm32_metadata_loader    = 0;
    uintptr_t arm32_metadata_cache_init= 0;
    uintptr_t arm32_domain_get         = 0;
    uintptr_t arm32_domain_get_assemblies = 0;
    uintptr_t arm32_image_get_class    = 0;
    uintptr_t arm32_thread_attach      = 0;
#endif
};

namespace known_hashes {
    inline constexpr std::array<uint32_t, 4> kDomainGet64     = {0xA1B2C3D4, 0xE5F60718, 0x293A4B5C, 0x6D7E8F90};
    inline constexpr std::array<uint32_t, 4> kMetadataLoader64= {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};
}

namespace arm64_prologue {
    inline constexpr std::array<uint8_t, 8> kDomainGet               = {0xE0,0x03,0x00,0xAA,0xC0,0x03,0x5F,0xD6};
    inline constexpr std::array<uint8_t, 8> kDomainGet2026           = {0x08,0x00,0x40,0xF9,0xE0,0x03,0x08,0xAA};
    inline constexpr std::array<uint8_t, 8> kDomainGetAssemblies     = {0xE8,0x03,0x00,0xF9,0xE9,0x03,0x01,0xF9};
    inline constexpr std::array<uint8_t, 8> kDomainGetAssemblies2024 = {0x08,0x00,0x40,0xF9,0xE8,0x03,0x01,0xF9};
    inline constexpr std::array<uint8_t, 8> kImageGetClass           = {0xE8,0x03,0x01,0xF9,0xE9,0x03,0x00,0xF9};
    inline constexpr std::array<uint8_t, 8> kImageGetClass2026       = {0x08,0x10,0x40,0xF9,0x28,0x00,0x40,0xF9};
    inline constexpr std::array<uint8_t, 8> kThreadAttach            = {0xE8,0x03,0x40,0xF9,0x08,0x01,0x40,0xF9};
    inline constexpr std::array<uint8_t, 8> kIsVmThread              = {0x08,0x00,0x40,0xF9,0x08,0x01,0x40,0xB9};
    inline constexpr std::array<uint8_t, 8> kMetadataLoader          = {0xFD,0x7B,0xBF,0xA9,0xFD,0x03,0x00,0x91};
    inline constexpr std::array<uint8_t, 8> kMetadataCacheInitialize64={0xFD,0x7B,0xBF,0xA9,0xF3,0x0F,0x1F,0xF8};
    inline constexpr std::array<uint8_t, 8> kMetadataDecrypt         = {0xFD,0x7B,0xBF,0xA9,0xF3,0x53,0xBF,0xA9};
    inline constexpr std::array<uint8_t, 8> kIl2cppInit2026          = {0xFD,0x7B,0xBF,0xA9,0xF4,0x4F,0xBF,0xA9};
}

#if defined(__arm__)
namespace arm32_prologue {
    inline constexpr std::array<uint8_t, 8> kMetadataLoader32         = {0x2D,0xE9,0xF0,0x47,0x00,0x20,0x90,0xE5};
    inline constexpr std::array<uint8_t, 8> kMetadataCacheInitialize32= {0x2D,0xE9,0xF8,0x4F,0x05,0x46,0x16,0x46};
    inline constexpr std::array<uint8_t, 8> kDomainGet32              = {0x10,0xB5,0x04,0x46,0x00,0x20,0x90,0xE5};
    inline constexpr std::array<uint8_t, 8> kDomainGetAssemblies32    = {0x2D,0xE9,0xF0,0x41,0x00,0x24,0x90,0xE5};
    inline constexpr std::array<uint8_t, 8> kImageGetClass32          = {0x10,0xB5,0x01,0x24,0x90,0xE5,0x1C,0x00};
    inline constexpr std::array<uint8_t, 8> kThreadAttach32           = {0x2D,0xE9,0x10,0x40,0x01,0x20,0x90,0xE5};
}
#endif

template<size_t N>
[[nodiscard]] static std::optional<uintptr_t> scanPattern(
    const MemoryRegion& region, const std::array<uint8_t, N>& pattern, size_t align = 4)
{
    const auto* base = reinterpret_cast<const uint8_t*>(region.base);
    for (size_t i = 0; i + N <= region.size; i += align) {
        if (memcmp(base + i, pattern.data(), N) == 0)
            return region.base + i;
    }
    return std::nullopt;
}

template<size_t N>
[[nodiscard]] static std::optional<uintptr_t> scanPatternFuzzy(
    const MemoryRegion& region, const std::array<uint8_t, N>& pattern, size_t align = 4)
{
    const auto* base = reinterpret_cast<const uint8_t*>(region.base);
    constexpr size_t half = N / 2;
    for (size_t i = 0; i + N <= region.size; i += align) {
        if (memcmp(base + i, pattern.data(), half) == 0)
            return region.base + i;
    }
    return std::nullopt;
}

[[nodiscard]] static std::optional<uintptr_t> scanByHashLattice(
    const MemoryRegion& region, const std::vector<uint32_t>& hashes)
{
    const auto* base = reinterpret_cast<const uint32_t*>(region.base);
    size_t count = region.size / sizeof(uint32_t);
    for (size_t i = 0; i < count; ++i) {
        for (auto h : hashes) {
            if (base[i] == h) return region.base + i * sizeof(uint32_t);
        }
    }
    return std::nullopt;
}

[[nodiscard]] static std::optional<MemoryRegion> resolveExecutableRegion(uintptr_t lib_base) {
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return std::nullopt;

    char line[512];
    std::optional<MemoryRegion> result;

    while (fgets(line, sizeof(line), maps)) {
        uintptr_t s = 0, e = 0;
        char perms[8] = {};
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %7s", &s, &e, perms) < 3) continue;
        if (perms[2] != 'x') continue;
        if (s < lib_base || s > lib_base + 0x10000000u) continue;
        result = MemoryRegion{s, e - s};
        break;
    }
    fclose(maps);
    return result;
}

[[nodiscard]] static bool isReadableAddress(uintptr_t address) noexcept {
    if (!address) return false;
    auto page    = static_cast<uintptr_t>(getpagesize());
    auto aligned = reinterpret_cast<void*>(address & ~(page - 1u));
    return msync(aligned, page, MS_ASYNC) == 0;
}

[[nodiscard]] static ResolvedSymbols scanAllSymbols(uintptr_t library_base) {
    auto region = resolveExecutableRegion(library_base);
    if (!region) {
        LOGE("scanner: region resolve failed 0x%" PRIxPTR, library_base);
        return {};
    }
    ResolvedSymbols s;

    auto adaptive_scan = [&](auto& field,
                              const auto& pat,
                              const std::vector<uint32_t>& hashes,
                              const char* name) {
        if (field) return;
        if (auto a = scanPattern(*region, pat)) {
            field = *a;
            LOGI("scanner[strict]: %s -> 0x%" PRIxPTR, name, field);
            return;
        }
        if (auto a = scanPatternFuzzy(*region, pat)) {
            field = *a;
            LOGI("scanner[fuzzy]: %s -> 0x%" PRIxPTR, name, field);
            return;
        }
        if (!hashes.empty()) {
            if (auto a = scanByHashLattice(*region, hashes)) {
                field = *a;
                LOGI("scanner[hash]: %s -> 0x%" PRIxPTR, name, field);
                return;
            }
        }
        LOGW("scanner: %s not found (all strategies exhausted)", name);
    };

    static const std::vector<uint32_t> dg_hashes(
        known_hashes::kDomainGet64.begin(), known_hashes::kDomainGet64.end());
    static const std::vector<uint32_t> ml_hashes(
        known_hashes::kMetadataLoader64.begin(), known_hashes::kMetadataLoader64.end());
    static const std::vector<uint32_t> no_hashes;

    adaptive_scan(s.domain_get,               arm64_prologue::kDomainGet,               dg_hashes, "il2cpp_domain_get");
    adaptive_scan(s.domain_get_assemblies,     arm64_prologue::kDomainGetAssemblies,     no_hashes, "il2cpp_domain_get_assemblies");
    adaptive_scan(s.image_get_class,           arm64_prologue::kImageGetClass,           no_hashes, "il2cpp_image_get_class");
    adaptive_scan(s.thread_attach,             arm64_prologue::kThreadAttach,            no_hashes, "il2cpp_thread_attach");
    adaptive_scan(s.is_vm_thread,              arm64_prologue::kIsVmThread,              no_hashes, "il2cpp_is_vm_thread");
    adaptive_scan(s.domain_get,                arm64_prologue::kDomainGet2026,           dg_hashes, "il2cpp_domain_get[2026]");
    adaptive_scan(s.domain_get_assemblies,     arm64_prologue::kDomainGetAssemblies2024, no_hashes, "il2cpp_domain_get_assemblies[2024]");
    adaptive_scan(s.image_get_class,           arm64_prologue::kImageGetClass2026,       no_hashes, "il2cpp_image_get_class[2026]");
    adaptive_scan(s.domain_get_2026,           arm64_prologue::kDomainGet2026,           dg_hashes, "domain_get_2026");
    adaptive_scan(s.metadata_loader,           arm64_prologue::kMetadataLoader,          ml_hashes, "metadata_loader");
    adaptive_scan(s.metadata_cache_initialize, arm64_prologue::kMetadataCacheInitialize64, ml_hashes, "metadata_cache_init[64]");
    adaptive_scan(s.metadata_decrypt,          arm64_prologue::kMetadataDecrypt,         no_hashes, "metadata_decrypt");
    adaptive_scan(s.il2cpp_init_2026,          arm64_prologue::kIl2cppInit2026,          no_hashes, "il2cpp_init[2026]");

#if defined(__arm__)
    auto scan32 = [&](auto& field, const auto& pat, const char* name) {
        if (field) return;
        if (auto a = scanPattern(*region, pat, 2)) {
            field = *a | 1u;
            LOGI("scanner[arm32/strict]: %s -> 0x%" PRIxPTR, name, field);
            return;
        }
        if (auto a = scanPatternFuzzy(*region, pat, 2)) {
            field = *a | 1u;
            LOGI("scanner[arm32/fuzzy]: %s -> 0x%" PRIxPTR, name, field);
            return;
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

static int dl_iterate_phdr_callback(struct dl_phdr_info* info, size_t, void* data) {
    if (info->dlpi_name && strstr(info->dlpi_name, "libil2cpp.so")) {
        *reinterpret_cast<uintptr_t*>(data) = static_cast<uintptr_t>(info->dlpi_addr);
        return 1;
    }
    return 0;
}

[[nodiscard]] static uintptr_t findLibBase(const char*) {
    uintptr_t base = 0;
    dl_iterate_phdr(dl_iterate_phdr_callback, &base);
    return base;
}

static void sigsegv_handler(int sig, siginfo_t* info, void* ucontext) {
    (void)ucontext;
    LOGE("SIGSEGV caught sig=%d addr=%p", sig, info ? info->si_addr : nullptr);
    struct sigaction sa{};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    raise(SIGSEGV);
}

} // namespace scanner

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
inline constexpr int kAndroidMaxApi = 37;

struct DumperConfig {
    std::string Target_Game;
    std::string Output            = "/sdcard/Eaquel_Dumps";
    bool        Cpp_Header        = true;
    bool        Frida_Script      = true;
    bool        include_generic   = true;
    bool        Protected_Breaker = true;
    bool        is_64bit          = (sizeof(void*) == 8);
};

[[nodiscard]] static std::optional<std::string> extractJsonString(
    const std::string& json, std::string_view key)
{
    std::string needle = "\"" + std::string(key) + "\"";
    size_t kp = json.find(needle);
    if (kp == std::string::npos) return std::nullopt;
    size_t cp = json.find(':', kp + needle.size());
    if (cp == std::string::npos) return std::nullopt;
    size_t vs = json.find_first_not_of(" \t\r\n", cp + 1);
    if (vs == std::string::npos || json[vs] != '"') return std::nullopt;
    std::string result;
    result.reserve(64);
    size_t i = vs + 1;
    while (i < json.size() && json[i] != '"') {
        if (json[i] == '\\' && i + 1 < json.size()) {
            ++i;
            switch (json[i]) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += json[i]; break;
            }
        } else { result += json[i]; }
        ++i;
    }
    return result;
}

[[nodiscard]] static bool extractJsonBool(
    const std::string& json, std::string_view key, bool def)
{
    std::string needle = "\"" + std::string(key) + "\"";
    size_t kp = json.find(needle);
    if (kp == std::string::npos) return def;
    size_t cp = json.find(':', kp + needle.size());
    if (cp == std::string::npos) return def;
    size_t vs = json.find_first_not_of(" \t\r\n", cp + 1);
    if (vs == std::string::npos) return def;
    if (json.compare(vs, 4, "true")  == 0) return true;
    if (json.compare(vs, 5, "false") == 0) return false;
    return def;
}

[[nodiscard]] static DumperConfig parseJsonConfig(const std::string& json) {
    DumperConfig cfg;
    cfg.Output   = std::string(kFallbackOutput);
    cfg.is_64bit = (sizeof(void*) == 8);
    if (auto v = extractJsonString(json, "Target_Game"); v) cfg.Target_Game = *v;
    if (auto v = extractJsonString(json, "Output"); v && !v->empty()) cfg.Output = *v;
    cfg.Cpp_Header        = extractJsonBool(json, "Cpp_Header",        true);
    cfg.Frida_Script      = extractJsonBool(json, "Frida_Script",      true);
    cfg.include_generic   = extractJsonBool(json, "include_generic",   true);
    cfg.Protected_Breaker = extractJsonBool(json, "Protected_Breaker", true);
    return cfg;
}

[[nodiscard]] static DumperConfig loadConfig() {
    const std::string_view paths[] = {
        kConfigPathPrimary, kConfigPathModule, kConfigPathKsu, kConfigPathApatch
    };
    for (auto path : paths) {
        std::ifstream file(path.data());
        if (!file.is_open()) continue;
        std::ostringstream buf; buf << file.rdbuf();
        auto json = buf.str();
        if (json.empty()) continue;
        auto cfg = parseJsonConfig(json);
        LOGI("config: loaded %s  Target_Game=%s  Output=%s  64bit=%d",
             path.data(), cfg.Target_Game.c_str(), cfg.Output.c_str(), (int)cfg.is_64bit);
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

[[nodiscard]] static bool isTargetPackage(const DumperConfig& cfg, std::string_view pkg) {
    if (isWildcardTarget(cfg))
        return true;
    return isExplicitTarget(cfg) && cfg.Target_Game == pkg;
}

using HotReloadCallback = std::function<void(const DumperConfig&)>;

class ConfigWatcher {
public:
    bool start(HotReloadCallback cb) {
        cb_  = std::move(cb);
        ifd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (ifd_ < 0) { LOGE("inotify_init1 failed errno=%d", errno); return false; }
        efd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (efd_ < 0) { LOGE("eventfd failed errno=%d", errno); ::close(ifd_); ifd_=-1; return false; }

        const std::string_view paths[] = {
            kConfigPathPrimary, kConfigPathModule, kConfigPathKsu, kConfigPathApatch
        };
        bool any_watch = false;
        for (auto p : paths) {
            std::string dir(p);
            auto slash = dir.rfind('/');
            if (slash != std::string::npos) dir.resize(slash);
            int wd = inotify_add_watch(ifd_, dir.c_str(),
                              IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE);
            if (wd >= 0) {
                any_watch = true;
                LOGI("inotify: watching %s (wd=%d)", dir.c_str(), wd);
            } else {
                LOGW("inotify: watch failed for %s errno=%d", dir.c_str(), errno);
            }
        }
        if (!any_watch) {
            LOGW("inotify: no directories watchable -- hot-reload disabled");
        }

        running_.store(true, std::memory_order_release);
        thread_ = std::thread([this]() { watchLoop(); });
        return true;
    }

    void stop() {
        running_.store(false, std::memory_order_release);
        if (efd_ >= 0) { uint64_t v = 1; (void)write(efd_, &v, 8); }
        if (thread_.joinable()) thread_.join();
        if (ifd_ >= 0) { ::close(ifd_); ifd_ = -1; }
        if (efd_ >= 0) { ::close(efd_); efd_ = -1; }
    }

    ~ConfigWatcher() { stop(); }

private:
    HotReloadCallback cb_;
    int               ifd_    = -1;
    int               efd_    = -1;
    std::atomic<bool> running_{false};
    std::thread       thread_;

    void watchLoop() {
        constexpr size_t kBufSz = sizeof(inotify_event) + NAME_MAX + 1;
        alignas(inotify_event) char buf[kBufSz * 4];

        struct pollfd fds[2];
        fds[0] = { ifd_, POLLIN, 0 };
        fds[1] = { efd_, POLLIN, 0 };

        while (running_.load(std::memory_order_acquire)) {
            int ret = poll(fds, 2, -1);
            if (ret <= 0) continue;
            if (fds[1].revents & POLLIN) break;
            if (!(fds[0].revents & POLLIN)) continue;
            ssize_t len = read(ifd_, buf, sizeof(buf));
            if (len <= 0) continue;

            bool reload = false;
            for (char* ptr = buf; ptr < buf + len; ) {
                auto* ev = reinterpret_cast<inotify_event*>(ptr);
                ptr += sizeof(inotify_event) + ev->len;
                if (ev->len == 0) { reload = true; continue; }
                if (strcmp(ev->name, "Eaquel_Config.json") == 0) {
                    reload = true;
                }
            }
            if (reload) {
                struct pollfd settle_pf = { ifd_, POLLIN, 0 };
                poll(&settle_pf, 1, 80);
                auto cfg = loadConfig();
                LOGI("config: hot-reload triggered -- Target_Game=%s Protected_Breaker=%d Output=%s",
                     cfg.Target_Game.c_str(), (int)cfg.Protected_Breaker, cfg.Output.c_str());
                if (cb_) cb_(cfg);
            }
        }
    }
};

} // namespace config
