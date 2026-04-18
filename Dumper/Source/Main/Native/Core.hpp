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
#include <sys/mman.h>
#include <unistd.h>
#include <link.h>
#include <signal.h>

#define LOG_TAG "Eaquel"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

inline constexpr uint32_t FIELD_ATTRIBUTE_FIELD_ACCESS_MASK    = 0x0007;
inline constexpr uint32_t FIELD_ATTRIBUTE_COMPILER_CONTROLLED  = 0x0000;
inline constexpr uint32_t FIELD_ATTRIBUTE_PRIVATE              = 0x0001;
inline constexpr uint32_t FIELD_ATTRIBUTE_FAM_AND_ASSEM        = 0x0002;
inline constexpr uint32_t FIELD_ATTRIBUTE_ASSEMBLY             = 0x0003;
inline constexpr uint32_t FIELD_ATTRIBUTE_FAMILY               = 0x0004;
inline constexpr uint32_t FIELD_ATTRIBUTE_FAM_OR_ASSEM         = 0x0005;
inline constexpr uint32_t FIELD_ATTRIBUTE_PUBLIC               = 0x0006;
inline constexpr uint32_t FIELD_ATTRIBUTE_STATIC               = 0x0010;
inline constexpr uint32_t FIELD_ATTRIBUTE_INIT_ONLY            = 0x0020;
inline constexpr uint32_t FIELD_ATTRIBUTE_LITERAL              = 0x0040;
inline constexpr uint32_t FIELD_ATTRIBUTE_NOT_SERIALIZED       = 0x0080;
inline constexpr uint32_t FIELD_ATTRIBUTE_SPECIAL_NAME         = 0x0200;
inline constexpr uint32_t FIELD_ATTRIBUTE_PINVOKE_IMPL         = 0x2000;
inline constexpr uint32_t FIELD_ATTRIBUTE_RESERVED_MASK        = 0x9500;
inline constexpr uint32_t FIELD_ATTRIBUTE_RT_SPECIAL_NAME      = 0x0400;
inline constexpr uint32_t FIELD_ATTRIBUTE_HAS_FIELD_MARSHAL    = 0x1000;
inline constexpr uint32_t FIELD_ATTRIBUTE_HAS_DEFAULT          = 0x8000;
inline constexpr uint32_t FIELD_ATTRIBUTE_HAS_FIELD_RVA        = 0x0100;

inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_CODE_TYPE_MASK      = 0x0003;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_IL                  = 0x0000;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_NATIVE              = 0x0001;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_OPTIL               = 0x0002;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_RUNTIME             = 0x0003;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_MANAGED_MASK        = 0x0004;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_UNMANAGED           = 0x0004;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_MANAGED             = 0x0000;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_FORWARD_REF         = 0x0010;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_PRESERVE_SIG        = 0x0080;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_INTERNAL_CALL       = 0x1000;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_SYNCHRONIZED        = 0x0020;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_NOINLINING          = 0x0008;
inline constexpr uint32_t METHOD_IMPL_ATTRIBUTE_MAX_METHOD_IMPL_VAL = 0xffff;

inline constexpr uint32_t METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK   = 0x0007;
inline constexpr uint32_t METHOD_ATTRIBUTE_COMPILER_CONTROLLED  = 0x0000;
inline constexpr uint32_t METHOD_ATTRIBUTE_PRIVATE              = 0x0001;
inline constexpr uint32_t METHOD_ATTRIBUTE_FAM_AND_ASSEM        = 0x0002;
inline constexpr uint32_t METHOD_ATTRIBUTE_ASSEM                = 0x0003;
inline constexpr uint32_t METHOD_ATTRIBUTE_FAMILY               = 0x0004;
inline constexpr uint32_t METHOD_ATTRIBUTE_FAM_OR_ASSEM         = 0x0005;
inline constexpr uint32_t METHOD_ATTRIBUTE_PUBLIC               = 0x0006;
inline constexpr uint32_t METHOD_ATTRIBUTE_STATIC               = 0x0010;
inline constexpr uint32_t METHOD_ATTRIBUTE_FINAL                = 0x0020;
inline constexpr uint32_t METHOD_ATTRIBUTE_VIRTUAL              = 0x0040;
inline constexpr uint32_t METHOD_ATTRIBUTE_HIDE_BY_SIG          = 0x0080;
inline constexpr uint32_t METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK   = 0x0100;
inline constexpr uint32_t METHOD_ATTRIBUTE_REUSE_SLOT           = 0x0000;
inline constexpr uint32_t METHOD_ATTRIBUTE_NEW_SLOT             = 0x0100;
inline constexpr uint32_t METHOD_ATTRIBUTE_STRICT               = 0x0200;
inline constexpr uint32_t METHOD_ATTRIBUTE_ABSTRACT             = 0x0400;
inline constexpr uint32_t METHOD_ATTRIBUTE_SPECIAL_NAME         = 0x0800;
inline constexpr uint32_t METHOD_ATTRIBUTE_PINVOKE_IMPL         = 0x2000;
inline constexpr uint32_t METHOD_ATTRIBUTE_UNMANAGED_EXPORT     = 0x0008;
inline constexpr uint32_t METHOD_ATTRIBUTE_RESERVED_MASK        = 0xd000;
inline constexpr uint32_t METHOD_ATTRIBUTE_RT_SPECIAL_NAME      = 0x1000;
inline constexpr uint32_t METHOD_ATTRIBUTE_HAS_SECURITY         = 0x4000;
inline constexpr uint32_t METHOD_ATTRIBUTE_REQUIRE_SEC_OBJECT   = 0x8000;

inline constexpr uint32_t TYPE_ATTRIBUTE_VISIBILITY_MASK        = 0x00000007;
inline constexpr uint32_t TYPE_ATTRIBUTE_NOT_PUBLIC             = 0x00000000;
inline constexpr uint32_t TYPE_ATTRIBUTE_PUBLIC                 = 0x00000001;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_PUBLIC          = 0x00000002;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_PRIVATE         = 0x00000003;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_FAMILY          = 0x00000004;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_ASSEMBLY        = 0x00000005;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM   = 0x00000006;
inline constexpr uint32_t TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM    = 0x00000007;
inline constexpr uint32_t TYPE_ATTRIBUTE_LAYOUT_MASK            = 0x00000018;
inline constexpr uint32_t TYPE_ATTRIBUTE_AUTO_LAYOUT            = 0x00000000;
inline constexpr uint32_t TYPE_ATTRIBUTE_SEQUENTIAL_LAYOUT      = 0x00000008;
inline constexpr uint32_t TYPE_ATTRIBUTE_EXPLICIT_LAYOUT        = 0x00000010;
inline constexpr uint32_t TYPE_ATTRIBUTE_CLASS_SEMANTIC_MASK    = 0x00000020;
inline constexpr uint32_t TYPE_ATTRIBUTE_CLASS                  = 0x00000000;
inline constexpr uint32_t TYPE_ATTRIBUTE_INTERFACE              = 0x00000020;
inline constexpr uint32_t TYPE_ATTRIBUTE_ABSTRACT               = 0x00000080;
inline constexpr uint32_t TYPE_ATTRIBUTE_SEALED                 = 0x00000100;
inline constexpr uint32_t TYPE_ATTRIBUTE_SPECIAL_NAME           = 0x00000400;
inline constexpr uint32_t TYPE_ATTRIBUTE_IMPORT                 = 0x00001000;
inline constexpr uint32_t TYPE_ATTRIBUTE_SERIALIZABLE           = 0x00002000;
inline constexpr uint32_t TYPE_ATTRIBUTE_STRING_FORMAT_MASK     = 0x00030000;
inline constexpr uint32_t TYPE_ATTRIBUTE_ANSI_CLASS             = 0x00000000;
inline constexpr uint32_t TYPE_ATTRIBUTE_UNICODE_CLASS          = 0x00010000;
inline constexpr uint32_t TYPE_ATTRIBUTE_AUTO_CLASS             = 0x00020000;
inline constexpr uint32_t TYPE_ATTRIBUTE_BEFORE_FIELD_INIT      = 0x00100000;
inline constexpr uint32_t TYPE_ATTRIBUTE_FORWARDER              = 0x00200000;
inline constexpr uint32_t TYPE_ATTRIBUTE_RESERVED_MASK          = 0x00040800;
inline constexpr uint32_t TYPE_ATTRIBUTE_RT_SPECIAL_NAME        = 0x00000800;
inline constexpr uint32_t TYPE_ATTRIBUTE_HAS_SECURITY           = 0x00040000;

inline constexpr uint32_t PARAM_ATTRIBUTE_IN                = 0x0001;
inline constexpr uint32_t PARAM_ATTRIBUTE_OUT               = 0x0002;
inline constexpr uint32_t PARAM_ATTRIBUTE_OPTIONAL          = 0x0010;
inline constexpr uint32_t PARAM_ATTRIBUTE_RESERVED_MASK     = 0xf000;
inline constexpr uint32_t PARAM_ATTRIBUTE_HAS_DEFAULT       = 0x1000;
inline constexpr uint32_t PARAM_ATTRIBUTE_HAS_FIELD_MARSHAL = 0x2000;
inline constexpr uint32_t PARAM_ATTRIBUTE_UNUSED            = 0xcfe0;

inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_NON_VARIANT                        = 0x00;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_COVARIANT                          = 0x01;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_CONTRAVARIANT                      = 0x02;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_VARIANCE_MASK                      = 0x03;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_REFERENCE_TYPE_CONSTRAINT          = 0x04;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_NOT_NULLABLE_VALUE_TYPE_CONSTRAINT = 0x08;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_DEFAULT_CONSTRUCTOR_CONSTRAINT     = 0x10;
inline constexpr uint32_t IL2CPP_GENERIC_PARAMETER_ATTRIBUTE_SPECIAL_CONSTRAINT_MASK            = 0x1C;

inline constexpr uint32_t ASSEMBLYREF_FULL_PUBLIC_KEY_FLAG             = 0x00000001;
inline constexpr uint32_t ASSEMBLYREF_RETARGETABLE_FLAG                = 0x00000100;
inline constexpr uint32_t ASSEMBLYREF_ENABLEJITCOMPILE_TRACKING_FLAG   = 0x00008000;
inline constexpr uint32_t ASSEMBLYREF_DISABLEJITCOMPILE_OPTIMIZER_FLAG = 0x00004000;

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

using Il2CppVTable = Il2CppClass;
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
    IL2CPP_STAT_NEW_OBJECT_COUNT,    IL2CPP_STAT_INITIALIZED_CLASS_COUNT,
    IL2CPP_STAT_METHOD_COUNT,        IL2CPP_STAT_CLASS_STATIC_DATA_SIZE,
    IL2CPP_STAT_GENERIC_INSTANCE_COUNT, IL2CPP_STAT_GENERIC_CLASS_COUNT,
    IL2CPP_STAT_INFLATED_METHOD_COUNT,  IL2CPP_STAT_INFLATED_TYPE_COUNT,
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

struct MethodInfo { Il2CppMethodPointer methodPointer; };

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
DO_API(int,  il2cpp_init,                            (const char* domain_name))
DO_API(int,  il2cpp_init_utf16,                      (const Il2CppChar* domain_name))
DO_API(void, il2cpp_shutdown,                        ())
DO_API(void, il2cpp_set_config_dir,                  (const char *configPath))
DO_API(void, il2cpp_set_data_dir,                    (const char *dataPath))
DO_API(void, il2cpp_set_temp_dir,                    (const char *tempPath))
DO_API(void, il2cpp_set_commandline_arguments,       (int argc, const char* const argv[], const char* basedir))
DO_API(void, il2cpp_set_commandline_arguments_utf16, (int argc, const Il2CppChar* const argv[], const char* basedir))
DO_API(void, il2cpp_set_config_utf16,                (const Il2CppChar * executablePath))
DO_API(void, il2cpp_set_config,                      (const char* executablePath))
DO_API(void, il2cpp_set_memory_callbacks,            (Il2CppMemoryCallbacks *callbacks))
DO_API(const Il2CppImage*,    il2cpp_get_corlib,           ())
DO_API(void,                  il2cpp_add_internal_call,    (const char* name, Il2CppMethodPointer method))
DO_API(Il2CppMethodPointer,   il2cpp_resolve_icall,        (const char* name))
DO_API(void*,                 il2cpp_alloc,                (size_t size))
DO_API(void,                  il2cpp_free,                 (void* ptr))
DO_API(Il2CppClass*,          il2cpp_array_class_get,      (Il2CppClass* element_class, uint32_t rank))
DO_API(uint32_t,              il2cpp_array_length,         (Il2CppArray* array))
DO_API(uint32_t,              il2cpp_array_get_byte_length,(Il2CppArray* array))
DO_API(Il2CppArray*,          il2cpp_array_new,            (Il2CppClass* elementTypeInfo, il2cpp_array_size_t length))
DO_API(Il2CppArray*,          il2cpp_array_new_specific,   (Il2CppClass* arrayTypeInfo, il2cpp_array_size_t length))
DO_API(Il2CppArray*,          il2cpp_array_new_full,       (Il2CppClass* array_class, il2cpp_array_size_t *lengths, il2cpp_array_size_t *lower_bounds))
DO_API(Il2CppClass*,          il2cpp_bounded_array_class_get,(Il2CppClass* element_class, uint32_t rank, bool bounded))
DO_API(int,                   il2cpp_array_element_size,   (const Il2CppClass* array_class))
DO_API(const Il2CppImage*,    il2cpp_assembly_get_image,   (const Il2CppAssembly* assembly))
DO_API(Il2CppClass*,          il2cpp_class_enum_basetype,  (Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_generic,     (const Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_inflated,    (const Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_assignable_from,(Il2CppClass* klass, Il2CppClass* oklass))
DO_API(bool,                  il2cpp_class_is_subclass_of, (Il2CppClass* klass, Il2CppClass* klassc, bool check_interfaces))
DO_API(bool,                  il2cpp_class_has_parent,     (Il2CppClass* klass, Il2CppClass* klassc))
DO_API(Il2CppClass*,          il2cpp_class_from_il2cpp_type,(const Il2CppType* type))
DO_API(Il2CppClass*,          il2cpp_class_from_name,      (const Il2CppImage* image, const char* namespaze, const char *name))
DO_API(Il2CppClass*,          il2cpp_class_from_system_type,(Il2CppReflectionType *type))
DO_API(Il2CppClass*,          il2cpp_class_get_element_class,(Il2CppClass* klass))
DO_API(const EventInfo*,      il2cpp_class_get_events,     (Il2CppClass* klass, void* *iter))
DO_API(FieldInfo*,            il2cpp_class_get_fields,     (Il2CppClass* klass, void* *iter))
DO_API(Il2CppClass*,          il2cpp_class_get_nested_types,(Il2CppClass* klass, void* *iter))
DO_API(Il2CppClass*,          il2cpp_class_get_interfaces, (Il2CppClass* klass, void* *iter))
DO_API(const PropertyInfo*,   il2cpp_class_get_properties, (Il2CppClass* klass, void* *iter))
DO_API(const PropertyInfo*,   il2cpp_class_get_property_from_name,(Il2CppClass* klass, const char *name))
DO_API(FieldInfo*,            il2cpp_class_get_field_from_name,(Il2CppClass* klass, const char *name))
DO_API(const MethodInfo*,     il2cpp_class_get_methods,    (Il2CppClass* klass, void* *iter))
DO_API(const MethodInfo*,     il2cpp_class_get_method_from_name,(const Il2CppClass* klass, const char* name, int argsCount))
DO_API(const char*,           il2cpp_class_get_name,       (Il2CppClass* klass))
DO_API(void,                  il2cpp_type_get_name_chunked,(const Il2CppType* type, void(*chunkedName)(const char* name, void* userData), void* userData))
DO_API(const char*,           il2cpp_class_get_namespace,  (Il2CppClass* klass))
DO_API(Il2CppClass*,          il2cpp_class_get_parent,     (Il2CppClass* klass))
DO_API(Il2CppClass*,          il2cpp_class_get_declaring_type,(Il2CppClass* klass))
DO_API(int,                   il2cpp_class_get_rank,       (Il2CppClass* klass))
DO_API(uint32_t,              il2cpp_class_get_flags,      (const Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_valuetype,   (const Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_enum,        (const Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_abstract,    (const Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_interface,   (const Il2CppClass* klass))
DO_API(int,                   il2cpp_class_get_type_token, (Il2CppClass* klass))
DO_API(bool,                  il2cpp_class_is_blittable,   (const Il2CppClass* klass))
DO_API(const Il2CppType*,     il2cpp_class_get_type,       (Il2CppClass* klass))
DO_API(Il2CppClass*,          il2cpp_class_from_type,      (const Il2CppType* type))
DO_API(Il2CppDomain*,         il2cpp_domain_get,           ())
DO_API(const Il2CppAssembly**, il2cpp_domain_get_assemblies,(const Il2CppDomain* domain, size_t* size))
DO_API(Il2CppString*,         il2cpp_string_new,           (const char* str))
DO_API(Il2CppThread*,         il2cpp_thread_attach,        (Il2CppDomain* domain))
DO_API(bool,                  il2cpp_is_vm_thread,         (Il2CppThread* thread))
DO_API(size_t,                il2cpp_image_get_class_count,(const Il2CppImage* image))
DO_API(const Il2CppClass*,    il2cpp_image_get_class,      (const Il2CppImage* image, size_t index))
DO_API(const char*,           il2cpp_image_get_name,       (const Il2CppImage* image))
DO_API(int,                   il2cpp_field_get_flags,      (FieldInfo* field))
DO_API(const char*,           il2cpp_field_get_name,       (FieldInfo* field))
DO_API(size_t,                il2cpp_field_get_offset,     (FieldInfo* field))
DO_API(const Il2CppType*,     il2cpp_field_get_type,       (FieldInfo* field))
DO_API(void,                  il2cpp_field_static_get_value,(FieldInfo* field, void* value))
DO_API(uint32_t,              il2cpp_property_get_flags,   (PropertyInfo* prop))
DO_API(const MethodInfo*,     il2cpp_property_get_get_method,(PropertyInfo* prop))
DO_API(const MethodInfo*,     il2cpp_property_get_set_method,(PropertyInfo* prop))
DO_API(const char*,           il2cpp_property_get_name,    (PropertyInfo* prop))
DO_API(const Il2CppType*,     il2cpp_method_get_return_type,(const MethodInfo* method))
DO_API(const char*,           il2cpp_method_get_name,      (const MethodInfo* method))
DO_API(uint32_t,              il2cpp_method_get_param_count,(const MethodInfo* method))
DO_API(const Il2CppType*,     il2cpp_method_get_param,     (const MethodInfo* method, uint32_t index))
DO_API(uint32_t,              il2cpp_method_get_flags,     (const MethodInfo* method, uint32_t* iflags))
DO_API(const char*,           il2cpp_method_get_param_name,(const MethodInfo* method, uint32_t index))
DO_API(bool,                  il2cpp_type_is_byref,        (const Il2CppType* type))
DO_API(bool,                  il2cpp_type_is_static,       (const Il2CppType* type))
DO_API(bool,                  il2cpp_type_is_pointer_type, (const Il2CppType* type))
DO_API(char*,                 il2cpp_type_get_name,        (const Il2CppType* type))
DO_API(int,                   il2cpp_type_get_type,        (const Il2CppType* type))
DO_API(void,                  il2cpp_set_default_thread_affinity,(int64_t affinity_mask))
#endif

namespace entropy {

inline constexpr uint32_t kIl2CppMetadataMagic  = 0xFAB11BAFu;
inline constexpr size_t   kEntropySampleSize     = 1024;
inline constexpr double   kHighEntropyThreshold  = 7.2;
inline constexpr double   kLowEntropyThreshold   = 4.5;

enum class MetadataState : uint8_t { Unknown, Plaintext, Encrypted, Compressed };

[[nodiscard]] static double shannonEntropy(const uint8_t* data, size_t len) {
    if (!data || len == 0) return 0.0;
    uint32_t freq[256] = {};
    for (size_t i = 0; i < len; ++i) ++freq[data[i]];
    double h = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (!freq[i]) continue;
        double p = static_cast<double>(freq[i]) / static_cast<double>(len);
        h -= p * std::log2(p);
    }
    return h;
}

[[nodiscard]] static MetadataState analyzeBuffer(const uint8_t* data, size_t len) {
    if (!data || len < 4) return MetadataState::Unknown;
    uint32_t magic = 0;
    memcpy(&magic, data, 4);
    if (magic == kIl2CppMetadataMagic) {
        LOGI("[Entropy] Magic 0xFAB11BAF confirmed — plaintext");
        return MetadataState::Plaintext;
    }
    size_t sample = len < kEntropySampleSize ? len : kEntropySampleSize;
    double h = shannonEntropy(data, sample);
    LOGI("[Entropy] Score=%.3f on first %zu bytes (magic=0x%08X)", h, sample, magic);
    if (h > kHighEntropyThreshold) return MetadataState::Encrypted;
    if (h < kLowEntropyThreshold)  return MetadataState::Plaintext;
    return MetadataState::Compressed;
}

struct XorKeyResult {
    bool    found    = false;
    uint8_t key[16]  = {};
    size_t  key_len  = 1;
    int     score    = 0;
};

static constexpr const char* kHeuristicStrings[] = {
    "System", "mscorlib", "UnityEngine", "Assembly-CSharp",
    "System.Collections", "System.IO", "UnityEngine.CoreModule",
    "System.Threading", "System.Text",
};
static constexpr size_t kHeuristicCount = sizeof(kHeuristicStrings) / sizeof(kHeuristicStrings[0]);

[[nodiscard]] static int scoreDecryptedBlock(const uint8_t* data, size_t len) {
    int score = 0;
    for (size_t k = 0; k < kHeuristicCount; ++k) {
        const char* needle = kHeuristicStrings[k];
        size_t      nlen   = strlen(needle);
        if (len < nlen) continue;
        for (size_t i = 0; i <= len - nlen; ++i) {
            if (memcmp(data + i, needle, nlen) == 0) { ++score; break; }
        }
    }
    return score;
}

[[nodiscard]] static XorKeyResult discoverXorKey(const uint8_t* enc, size_t len) {
    XorKeyResult res;
    if (!enc || len < 4) return res;

    const uint8_t expected[4] = {
        uint8_t(kIl2CppMetadataMagic & 0xFF),
        uint8_t((kIl2CppMetadataMagic >> 8)  & 0xFF),
        uint8_t((kIl2CppMetadataMagic >> 16) & 0xFF),
        uint8_t((kIl2CppMetadataMagic >> 24) & 0xFF),
    };
    uint8_t delta[4];
    for (int i = 0; i < 4; ++i) delta[i] = enc[i] ^ expected[i];
    LOGI("[XorKey] Delta from magic: %02X %02X %02X %02X", delta[0], delta[1], delta[2], delta[3]);

    if (delta[0] == delta[1] && delta[1] == delta[2] && delta[2] == delta[3]) {
        res.found   = true;
        res.key[0]  = delta[0];
        res.key_len = 1;
        LOGI("[XorKey] 1-byte key: 0x%02X", res.key[0]);
    } else {
        memcpy(res.key, delta, 4);
        res.key_len = 4;
        res.found   = true;
        LOGI("[XorKey] 4-byte key: %02X %02X %02X %02X", delta[0], delta[1], delta[2], delta[3]);
    }

    constexpr size_t kKasiskiSample = 4096;
    size_t sample = len < kKasiskiSample ? len : kKasiskiSample;
    if (res.found && res.key_len <= 4 && len >= 8) {
        for (size_t period = 2; period <= 16; ++period) {
            bool diff_all = true;
            for (size_t i = period; i < sample && i < 64; ++i) {
                if ((enc[i] ^ enc[i - period]) == 0) { diff_all = false; break; }
            }
            if (diff_all) LOGI("[XorKey] Kasiski hint: period=%zu possible rolling key", period);
        }
    }

    if (res.found) {
        std::vector<uint8_t> dec(sample);
        for (size_t i = 0; i < sample; ++i) dec[i] = enc[i] ^ res.key[i % res.key_len];
        res.score = scoreDecryptedBlock(dec.data(), sample);
        LOGI("[XorKey] Heuristic score=%d", res.score);
        if (res.score < 2) LOGW("[XorKey] Low score — key may be wrong");
    }
    return res;
}

[[nodiscard]] static std::vector<uint8_t> decryptBuffer(
    const uint8_t* data, size_t len, const XorKeyResult& key
) {
    std::vector<uint8_t> out(len);
    if (!key.found || key.key_len == 0) { memcpy(out.data(), data, len); return out; }
    for (size_t i = 0; i < len; ++i) out[i] = data[i] ^ key.key[i % key.key_len];
    return out;
}

}

namespace scanner {

struct PatternByte { uint8_t value; bool is_wildcard; };
template<size_t N> using Pattern = std::array<PatternByte, N>;

namespace arm64_prologue {

inline constexpr auto kDomainGet = Pattern<8>{{
    {0xFF,false},{0x43,false},{0x00,false},{0xD1,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kDomainGetAssemblies = Pattern<12>{{
    {0xFF,false},{0x83,false},{0x00,false},{0xD1,false},
    {0xF8,false},{0x5F,false},{0x00,false},{0xA9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kImageGetClass = Pattern<8>{{
    {0xFF,false},{0x43,false},{0x01,false},{0xD1,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kThreadAttach = Pattern<8>{{
    {0xFF,false},{0x03,false},{0x01,false},{0xD1,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kIsVmThread = Pattern<8>{{
    {0xE8,false},{0x03,false},{0x00,false},{0xAA,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kDomainGet2026 = Pattern<12>{{
    {0xFF,false},{0x43,false},{0x00,false},{0xD1,false},
    {0xF0,false},{0x7B,false},{0x00,false},{0xF9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kIl2cppInit2026 = Pattern<16>{{
    {0xFF,false},{0x83,false},{0x00,false},{0xD1,false},
    {0xFD,false},{0x7B,false},{0x00,false},{0xF9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true },
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kDomainGetAssemblies2024 = Pattern<12>{{
    {0xFF,false},{0xC3,false},{0x00,false},{0xD1,false},
    {0xF8,false},{0x5F,false},{0x00,false},{0xA9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kImageGetClass2026 = Pattern<12>{{
    {0xFF,false},{0x43,false},{0x01,false},{0xD1,false},
    {0xFD,false},{0x7B,false},{0x00,false},{0xF9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kMetadataLoader = Pattern<16>{{
    {0xFF,false},{0x83,false},{0x01,false},{0xD1,false},
    {0xF4,false},{0x4F,false},{0x00,false},{0xA9,false},
    {0xFD,false},{0x7B,false},{0x00,false},{0xA9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kMetadataCacheInitialize64 = Pattern<20>{{
    {0xFF,false},{0x83,false},{0x01,false},{0xD1,false},
    {0xFD,false},{0x7B,false},{0x00,false},{0xF9,false},
    {0xF4,false},{0x4F,false},{0x00,false},{0xA9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true },
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kMetadataDecrypt = Pattern<12>{{
    {0xFF,false},{0x43,false},{0x02,false},{0xD1,false},
    {0xFD,false},{0x7B,false},{0x03,false},{0xA9,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};

}

namespace arm32_prologue {

inline constexpr auto kMetadataLoader32 = Pattern<8>{{
    {0x2D,false},{0xE9,false},{0xF0,false},{0x4F,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kMetadataCacheInitialize32 = Pattern<12>{{
    {0x2D,false},{0xE9,false},{0xF0,false},{0x5F,false},
    {0x00,false},{0xB5,false},{0x00,false},{0xBF,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kDomainGet32 = Pattern<8>{{
    {0x2D,false},{0xE9,false},{0xF0,false},{0x41,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kDomainGetAssemblies32 = Pattern<8>{{
    {0x2D,false},{0xE9,false},{0x70,false},{0x41,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kImageGetClass32 = Pattern<8>{{
    {0x2D,false},{0xE9,false},{0x30,false},{0x40,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};
inline constexpr auto kThreadAttach32 = Pattern<8>{{
    {0x2D,false},{0xE9,false},{0xF0,false},{0x47,false},
    {0x00,true },{0x00,true },{0x00,true },{0x00,true }
}};

}

struct MappedRegion { uintptr_t base; size_t size; };

struct ResolvedSymbols {
    uintptr_t domain_get                  = 0;
    uintptr_t domain_get_assemblies       = 0;
    uintptr_t image_get_class             = 0;
    uintptr_t thread_attach               = 0;
    uintptr_t is_vm_thread                = 0;
    uintptr_t domain_get_2026             = 0;
    uintptr_t metadata_loader             = 0;
    uintptr_t metadata_cache_initialize   = 0;
    uintptr_t metadata_decrypt            = 0;
    uintptr_t il2cpp_init_2026            = 0;
    uintptr_t domain_get_assemblies_2024  = 0;
    uintptr_t image_get_class_2026        = 0;
    uintptr_t arm32_metadata_loader       = 0;
    uintptr_t arm32_metadata_cache_init   = 0;
    uintptr_t arm32_domain_get            = 0;
    uintptr_t arm32_domain_get_assemblies = 0;
    uintptr_t arm32_image_get_class       = 0;
    uintptr_t arm32_thread_attach         = 0;
};

[[nodiscard]] static std::optional<MappedRegion> resolveExecutableRegion(uintptr_t base) {
    constexpr size_t kMaxScanSize = 0x8000000;
    auto page = static_cast<uintptr_t>(getpagesize());
    return MappedRegion{ base & ~(page - 1), kMaxScanSize };
}

template<size_t N>
[[nodiscard]] static std::optional<uintptr_t> scanPattern(
    const MappedRegion& region, const Pattern<N>& pattern, size_t alignment = 4
) {
    auto* mem   = reinterpret_cast<const uint8_t*>(region.base);
    auto  limit = region.size - N;
    for (size_t off = 0; off < limit; off += alignment) {
        bool matched = true;
        for (size_t i = 0; i < N; ++i) {
            if (!pattern[i].is_wildcard && mem[off + i] != pattern[i].value) {
                matched = false; break;
            }
        }
        if (matched) return region.base + off;
    }
    return std::nullopt;
}

[[nodiscard]] static bool isReadableAddress(uintptr_t address) {
    if (address == 0) return false;
    auto page    = static_cast<uintptr_t>(getpagesize());
    auto aligned = reinterpret_cast<void*>(address & ~(page - 1));
    return msync(aligned, page, MS_ASYNC) == 0;
}

[[nodiscard]] static ResolvedSymbols scanAllSymbols(uintptr_t library_base) {
    auto region = resolveExecutableRegion(library_base);
    if (!region) { LOGE("scanner: region resolve failed 0x%" PRIxPTR, library_base); return {}; }
    ResolvedSymbols s;
    auto try_scan = [&](auto& field, const auto& pat, const char* name) {
        if (field) return;
        if (auto a = scanPattern(*region, pat)) {
            field = *a; LOGI("scanner: %s -> 0x%" PRIxPTR, name, *a);
        } else { LOGW("scanner: %s not found", name); }
    };
    try_scan(s.domain_get,                arm64_prologue::kDomainGet,               "il2cpp_domain_get");
    try_scan(s.domain_get_assemblies,     arm64_prologue::kDomainGetAssemblies,     "il2cpp_domain_get_assemblies");
    try_scan(s.image_get_class,           arm64_prologue::kImageGetClass,           "il2cpp_image_get_class");
    try_scan(s.thread_attach,             arm64_prologue::kThreadAttach,            "il2cpp_thread_attach");
    try_scan(s.is_vm_thread,              arm64_prologue::kIsVmThread,              "il2cpp_is_vm_thread");
    try_scan(s.domain_get,                arm64_prologue::kDomainGet2026,           "il2cpp_domain_get[2026]");
    try_scan(s.domain_get_assemblies,     arm64_prologue::kDomainGetAssemblies2024, "il2cpp_domain_get_assemblies[2024]");
    try_scan(s.image_get_class,           arm64_prologue::kImageGetClass2026,       "il2cpp_image_get_class[2026]");
    try_scan(s.domain_get_2026,           arm64_prologue::kDomainGet2026,           "domain_get_2026");
    try_scan(s.metadata_loader,           arm64_prologue::kMetadataLoader,          "metadata_loader");
    try_scan(s.metadata_cache_initialize, arm64_prologue::kMetadataCacheInitialize64,"metadata_cache_init[64]");
    try_scan(s.metadata_decrypt,          arm64_prologue::kMetadataDecrypt,         "metadata_decrypt");
    try_scan(s.il2cpp_init_2026,          arm64_prologue::kIl2cppInit2026,          "il2cpp_init[2026]");
#if defined(__arm__)
    auto try_scan32 = [&](auto& field, const auto& pat, const char* name) {
        if (field) return;
        if (auto a = scanPattern(*region, pat, 2)) {
            field = *a | 1u; LOGI("scanner[arm32]: %s -> 0x%" PRIxPTR, name, field);
        } else { LOGW("scanner[arm32]: %s not found", name); }
    };
    try_scan32(s.arm32_metadata_loader,       arm32_prologue::kMetadataLoader32,          "metadata_loader[arm32]");
    try_scan32(s.arm32_metadata_cache_init,   arm32_prologue::kMetadataCacheInitialize32, "metadata_cache_init[arm32]");
    try_scan32(s.arm32_domain_get,            arm32_prologue::kDomainGet32,               "domain_get[arm32]");
    try_scan32(s.arm32_domain_get_assemblies, arm32_prologue::kDomainGetAssemblies32,     "domain_get_assemblies[arm32]");
    try_scan32(s.arm32_image_get_class,       arm32_prologue::kImageGetClass32,           "image_get_class[arm32]");
    try_scan32(s.arm32_thread_attach,         arm32_prologue::kThreadAttach32,            "thread_attach[arm32]");
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

}

namespace config {

inline constexpr std::string_view kConfigPathPrimary = "/data/local/tmp/Eaquel_Config.json";
inline constexpr std::string_view kConfigPathModule  = "/data/adb/modules/eaqueldumper/Eaquel_Config.json";
inline constexpr std::string_view kConfigPathKsu     = "/data/adb/ksu/modules/eaqueldumper/Eaquel_Config.json";
inline constexpr std::string_view kFallbackOutput    = "/sdcard/EaquelDumps";
inline constexpr std::string_view kFallbackOutputAlt = "/data/local/tmp/EaquelDumps";
inline constexpr std::string_view kCacheDir          = "/data/local/tmp/.eaq_cache";

struct DumperConfig {
    std::string target_package;
    std::string output_dir                 = "/sdcard/EaquelDumps";
    bool        generate_cpp_header        = true;
    bool        generate_frida_script      = true;
    bool        dump_nested_classes        = true;
    bool        include_generic            = true;
    bool        enable_early_metadata_hook = true;
    bool        force_32bit_mode           = false;
    bool        dump_only_cs               = false;
    bool        auto_detect_bit            = true;
};

[[nodiscard]] static std::optional<std::string> extractJsonString(
    const std::string& json, std::string_view key
) {
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
    const std::string& json, std::string_view key, bool def
) {
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
    cfg.output_dir = std::string(kFallbackOutput);
    if (auto v = extractJsonString(json, "target_package"); v && !v->empty()) cfg.target_package = *v;
    if (auto v = extractJsonString(json, "outputDir");      v && !v->empty()) cfg.output_dir     = *v;
    if (auto v = extractJsonString(json, "output_dir");     v && !v->empty()) cfg.output_dir     = *v;
    cfg.generate_cpp_header        = extractJsonBool(json, "generate_cpp_header",        true)
                                   || extractJsonBool(json, "genCpp",                    true);
    cfg.generate_frida_script      = extractJsonBool(json, "generate_frida_script",      true)
                                   || extractJsonBool(json, "genFrida",                  true);
    cfg.dump_nested_classes        = extractJsonBool(json, "dump_nested_classes",        true);
    cfg.include_generic            = extractJsonBool(json, "include_generic",            true);
    cfg.enable_early_metadata_hook = extractJsonBool(json, "enable_early_metadata_hook", true);
    cfg.force_32bit_mode           = extractJsonBool(json, "force_32bit_mode",           false);
    cfg.dump_only_cs               = extractJsonBool(json, "dump_only_cs",               false);
    cfg.auto_detect_bit            = extractJsonBool(json, "auto_detect_bit",            true);
    return cfg;
}

[[nodiscard]] static DumperConfig loadConfig() {
    const std::string_view paths[] = { kConfigPathPrimary, kConfigPathModule, kConfigPathKsu };
    for (auto path : paths) {
        std::ifstream file(path.data());
        if (!file.is_open()) continue;
        std::ostringstream buf; buf << file.rdbuf();
        auto json = buf.str();
        if (json.empty()) continue;
        auto cfg = parseJsonConfig(json);
        LOGI("config: %s target=%s output=%s", path.data(), cfg.target_package.c_str(), cfg.output_dir.c_str());
        return cfg;
    }
    LOGW("config: Eaquel_Config.json not found, using defaults");
    DumperConfig cfg; cfg.output_dir = std::string(kFallbackOutput); return cfg;
}

[[nodiscard]] static bool isTargetPackage(const DumperConfig& cfg, std::string_view pkg) {
    return !cfg.target_package.empty() && cfg.target_package == pkg;
}

}
