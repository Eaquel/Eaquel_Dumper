#pragma once

#include <android/log.h>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <cstring>
#include <optional>
#include <array>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <sys/mman.h>
#include <unistd.h>

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
    IL2CPP_TYPE_END        = 0x00, IL2CPP_TYPE_VOID       = 0x01,
    IL2CPP_TYPE_BOOLEAN    = 0x02, IL2CPP_TYPE_CHAR       = 0x03,
    IL2CPP_TYPE_I1         = 0x04, IL2CPP_TYPE_U1         = 0x05,
    IL2CPP_TYPE_I2         = 0x06, IL2CPP_TYPE_U2         = 0x07,
    IL2CPP_TYPE_I4         = 0x08, IL2CPP_TYPE_U4         = 0x09,
    IL2CPP_TYPE_I8         = 0x0a, IL2CPP_TYPE_U8         = 0x0b,
    IL2CPP_TYPE_R4         = 0x0c, IL2CPP_TYPE_R8         = 0x0d,
    IL2CPP_TYPE_STRING     = 0x0e, IL2CPP_TYPE_PTR        = 0x0f,
    IL2CPP_TYPE_BYREF      = 0x10, IL2CPP_TYPE_VALUETYPE  = 0x11,
    IL2CPP_TYPE_CLASS      = 0x12, IL2CPP_TYPE_VAR        = 0x13,
    IL2CPP_TYPE_ARRAY      = 0x14, IL2CPP_TYPE_GENERICINST = 0x15,
    IL2CPP_TYPE_TYPEDBYREF = 0x16, IL2CPP_TYPE_I          = 0x18,
    IL2CPP_TYPE_U          = 0x19, IL2CPP_TYPE_FNPTR      = 0x1b,
    IL2CPP_TYPE_OBJECT     = 0x1c, IL2CPP_TYPE_SZARRAY    = 0x1d,
    IL2CPP_TYPE_MVAR       = 0x1e, IL2CPP_TYPE_CMOD_REQD  = 0x1f,
    IL2CPP_TYPE_CMOD_OPT   = 0x20, IL2CPP_TYPE_INTERNAL   = 0x21,
    IL2CPP_TYPE_MODIFIER   = 0x40, IL2CPP_TYPE_SENTINEL   = 0x41,
    IL2CPP_TYPE_PINNED     = 0x45, IL2CPP_TYPE_ENUM       = 0x55,
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
DO_API(int32_t,      il2cpp_class_instance_size,   (Il2CppClass* klass))
DO_API(size_t,       il2cpp_class_num_fields,       (const Il2CppClass* enumKlass))
DO_API(bool,         il2cpp_class_is_valuetype,    (const Il2CppClass* klass))
DO_API(int32_t,      il2cpp_class_value_size,      (Il2CppClass* klass, uint32_t* align))
DO_API(bool,         il2cpp_class_is_blittable,    (const Il2CppClass* klass))
DO_API(int,          il2cpp_class_get_flags,       (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_abstract,     (const Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_interface,    (const Il2CppClass* klass))
DO_API(int,          il2cpp_class_array_element_size, (const Il2CppClass* klass))
DO_API(Il2CppClass*, il2cpp_class_from_type,       (const Il2CppType* type))
DO_API(const Il2CppType*, il2cpp_class_get_type,   (Il2CppClass* klass))
DO_API(uint32_t,     il2cpp_class_get_type_token,  (Il2CppClass* klass))
DO_API(bool,         il2cpp_class_has_attribute,   (Il2CppClass* klass, Il2CppClass* attr_class))
DO_API(bool,         il2cpp_class_has_references,  (Il2CppClass* klass))
DO_API(bool,         il2cpp_class_is_enum,         (const Il2CppClass* klass))
DO_API(const Il2CppImage*, il2cpp_class_get_image, (Il2CppClass* klass))
DO_API(const char*,  il2cpp_class_get_assemblyname, (const Il2CppClass* klass))
DO_API(int,          il2cpp_class_get_rank,        (const Il2CppClass* klass))
DO_API(uint32_t,     il2cpp_class_get_data_size,   (const Il2CppClass* klass))
DO_API(void*,        il2cpp_class_get_static_field_data, (const Il2CppClass* klass))
DO_API(size_t,       il2cpp_class_get_bitmap_size, (const Il2CppClass* klass))
DO_API(void,         il2cpp_class_get_bitmap,      (Il2CppClass* klass, size_t* bitmap))
DO_API(bool,         il2cpp_stats_get_value,       (Il2CppStat stat, size_t* value))
DO_API(Il2CppDomain*, il2cpp_domain_get,           ())
DO_API(const Il2CppAssembly*, il2cpp_domain_assembly_open, (Il2CppDomain* domain, const char* name))
DO_API(const Il2CppAssembly**, il2cpp_domain_get_assemblies, (const Il2CppDomain* domain, size_t* size))
DO_API(void,         il2cpp_raise_exception,       (Il2CppException* ex))
DO_API(Il2CppException*, il2cpp_exception_from_name_msg, (const Il2CppImage* image, const char *name_space, const char *name, const char *msg))
DO_API(Il2CppException*, il2cpp_get_exception_argument_null, (const char *arg))
DO_API(void,         il2cpp_format_exception,      (const Il2CppException* ex, char* message, int message_size))
DO_API(void,         il2cpp_format_stack_trace,    (const Il2CppException* ex, char* output, int output_size))
DO_API(void,         il2cpp_unhandled_exception,   (Il2CppException* ex))
DO_API(int,          il2cpp_field_get_flags,       (FieldInfo* field))
DO_API(const char*,  il2cpp_field_get_name,        (FieldInfo* field))
DO_API(Il2CppClass*, il2cpp_field_get_parent,      (FieldInfo* field))
DO_API(size_t,       il2cpp_field_get_offset,      (FieldInfo* field))
DO_API(const Il2CppType*, il2cpp_field_get_type,   (FieldInfo* field))
DO_API(void,         il2cpp_field_get_value,       (Il2CppObject* obj, FieldInfo* field, void* value))
DO_API(Il2CppObject*, il2cpp_field_get_value_object, (FieldInfo* field, Il2CppObject* obj))
DO_API(bool,         il2cpp_field_has_attribute,   (FieldInfo* field, Il2CppClass* attr_class))
DO_API(void,         il2cpp_field_set_value,       (Il2CppObject* obj, FieldInfo* field, void* value))
DO_API(void,         il2cpp_field_static_get_value,(FieldInfo* field, void* value))
DO_API(void,         il2cpp_field_static_set_value,(FieldInfo* field, void* value))
DO_API(void,         il2cpp_field_set_value_object,(Il2CppObject* instance, FieldInfo* field, Il2CppObject* value))
DO_API(bool,         il2cpp_field_is_literal,      (FieldInfo* field))
DO_API(void,         il2cpp_gc_collect,            (int maxGenerations))
DO_API(int32_t,      il2cpp_gc_collect_a_little,   ())
DO_API(void,         il2cpp_gc_disable,            ())
DO_API(void,         il2cpp_gc_enable,             ())
DO_API(bool,         il2cpp_gc_is_disabled,        ())
DO_API(Il2CppGCMode, il2cpp_gc_get_mode,           ())
DO_API(bool,         il2cpp_gc_set_mode,           (Il2CppGCMode mode))
DO_API(int64_t,      il2cpp_gc_get_used_size,      ())
DO_API(int64_t,      il2cpp_gc_get_heap_size,      ())
DO_API(void,         il2cpp_gc_wbarrier_set_field, (Il2CppObject* obj, void** targetAddress, void* object))
DO_API(bool,         il2cpp_gc_has_strict_wbarriers, ())
DO_API(void,         il2cpp_gc_set_external_allocation_tracker, (void(*)(void*, size_t, int)))
DO_API(void,         il2cpp_gc_set_external_wbarrier_tracker,   (void(*)(void**)))
DO_API(void,         il2cpp_gc_foreach_heap,       (void(*)(void* data, void* userData), void* userData))
DO_API(void,         il2cpp_stop_gc_world,         ())
DO_API(void,         il2cpp_start_gc_world,        ())
DO_API(void*,        il2cpp_gchandle_new,          (Il2CppObject* obj, bool pinned))
DO_API(void*,        il2cpp_gchandle_new_weakref,  (Il2CppObject* obj, bool track_resurrection))
DO_API(Il2CppObject*, il2cpp_gchandle_get_target,  (void* gchandle))
DO_API(void,         il2cpp_gchandle_free,         (void* gchandle))
DO_API(void,         il2cpp_gchandle_foreach_get_target, (void(*)(void* data, void* userData), void* userData))
DO_API(void*,        il2cpp_unity_liveness_calculation_begin, (Il2CppClass* filter, int max_object_count, il2cpp_register_object_callback callback, void* userdata, il2cpp_liveness_reallocate_callback reallocate))
DO_API(void,         il2cpp_unity_liveness_calculation_end,   (void* state))
DO_API(void,         il2cpp_unity_liveness_calculation_from_root, (Il2CppObject* root, void* state))
DO_API(void,         il2cpp_unity_liveness_calculation_from_statics, (void* state))
DO_API(const Il2CppType*, il2cpp_method_get_return_type, (const MethodInfo* method))
DO_API(Il2CppClass*,      il2cpp_method_get_declaring_type, (const MethodInfo* method))
DO_API(const char*,       il2cpp_method_get_name,       (const MethodInfo* method))
DO_API(const MethodInfo*, il2cpp_method_get_from_reflection, (const Il2CppReflectionMethod* method))
DO_API(Il2CppReflectionMethod*, il2cpp_method_get_object, (const MethodInfo* method, Il2CppClass* refclass))
DO_API(bool,              il2cpp_method_is_generic,     (const MethodInfo* method))
DO_API(bool,              il2cpp_method_is_inflated,    (const MethodInfo* method))
DO_API(bool,              il2cpp_method_is_instance,    (const MethodInfo* method))
DO_API(uint32_t,          il2cpp_method_get_param_count,(const MethodInfo* method))
DO_API(const Il2CppType*, il2cpp_method_get_param,      (const MethodInfo* method, uint32_t index))
DO_API(Il2CppClass*,      il2cpp_method_get_class,      (const MethodInfo* method))
DO_API(bool,              il2cpp_method_has_attribute,  (const MethodInfo* method, Il2CppClass* attr_class))
DO_API(uint32_t,          il2cpp_method_get_flags,      (const MethodInfo* method, uint32_t* iflags))
DO_API(uint32_t,          il2cpp_method_get_token,      (const MethodInfo* method))
DO_API(const char*,       il2cpp_method_get_param_name, (const MethodInfo* method, uint32_t index))
DO_API(uint32_t,          il2cpp_property_get_flags,    (PropertyInfo* prop))
DO_API(const MethodInfo*, il2cpp_property_get_get_method, (PropertyInfo* prop))
DO_API(const MethodInfo*, il2cpp_property_get_set_method, (PropertyInfo* prop))
DO_API(const char*,       il2cpp_property_get_name,     (PropertyInfo* prop))
DO_API(Il2CppClass*,      il2cpp_property_get_parent,   (PropertyInfo* prop))
DO_API(Il2CppClass*,      il2cpp_object_get_class,      (Il2CppObject* obj))
DO_API(uint32_t,          il2cpp_object_get_size,       (Il2CppObject* obj))
DO_API(const MethodInfo*, il2cpp_object_get_virtual_method, (Il2CppObject* obj, const MethodInfo* method))
DO_API(Il2CppObject*,     il2cpp_object_new,            (const Il2CppClass* klass))
DO_API(void*,             il2cpp_object_unbox,          (Il2CppObject* obj))
DO_API(Il2CppObject*,     il2cpp_value_box,             (Il2CppClass* klass, void* data))
DO_API(void,              il2cpp_monitor_enter,         (Il2CppObject* obj))
DO_API(bool,              il2cpp_monitor_try_enter,     (Il2CppObject* obj, uint32_t timeout))
DO_API(void,              il2cpp_monitor_exit,          (Il2CppObject* obj))
DO_API(void,              il2cpp_monitor_pulse,         (Il2CppObject* obj))
DO_API(void,              il2cpp_monitor_pulse_all,     (Il2CppObject* obj))
DO_API(void,              il2cpp_monitor_wait,          (Il2CppObject* obj))
DO_API(bool,              il2cpp_monitor_try_wait,      (Il2CppObject* obj, uint32_t timeout))
DO_API(Il2CppObject*,     il2cpp_runtime_invoke,        (const MethodInfo* method, void* obj, void** params, Il2CppException** exc))
DO_API(Il2CppObject*,     il2cpp_runtime_invoke_convert_args, (const MethodInfo* method, void* obj, Il2CppObject** params, int paramCount, Il2CppException** exc))
DO_API(void,              il2cpp_runtime_class_init,    (Il2CppClass* klass))
DO_API(void,              il2cpp_runtime_object_init,   (Il2CppObject* obj))
DO_API(void,              il2cpp_runtime_object_init_exception, (Il2CppObject* obj, Il2CppException** exc))
DO_API(void,              il2cpp_runtime_unhandled_exception_policy_set, (Il2CppRuntimeUnhandledExceptionPolicy value))
DO_API(int32_t,           il2cpp_string_length,         (Il2CppString* str))
DO_API(Il2CppChar*,       il2cpp_string_chars,          (Il2CppString* str))
DO_API(Il2CppString*,     il2cpp_string_new,            (const char* str))
DO_API(Il2CppString*,     il2cpp_string_new_len,        (const char* str, uint32_t length))
DO_API(Il2CppString*,     il2cpp_string_new_utf16,      (const Il2CppChar* text, int32_t len))
DO_API(Il2CppString*,     il2cpp_string_new_wrapper,    (const char* str))
DO_API(Il2CppString*,     il2cpp_string_intern,         (Il2CppString* str))
DO_API(Il2CppString*,     il2cpp_string_is_interned,    (Il2CppString* str))
DO_API(Il2CppThread*,     il2cpp_thread_current,        ())
DO_API(Il2CppThread*,     il2cpp_thread_attach,         (Il2CppDomain* domain))
DO_API(void,              il2cpp_thread_detach,         (Il2CppThread* thread))
DO_API(Il2CppThread**,    il2cpp_thread_get_all_attached_threads, (size_t* size))
DO_API(bool,              il2cpp_is_vm_thread,          (Il2CppThread* thread))
DO_API(void,              il2cpp_current_thread_walk_frame_stack, (Il2CppFrameWalkFunc func, void* user_data))
DO_API(void,              il2cpp_thread_walk_frame_stack, (Il2CppThread* thread, Il2CppFrameWalkFunc func, void* user_data))
DO_API(bool,              il2cpp_current_thread_get_top_frame, (Il2CppStackFrameInfo* frame))
DO_API(bool,              il2cpp_thread_get_top_frame,  (Il2CppThread* thread, Il2CppStackFrameInfo* frame))
DO_API(bool,              il2cpp_current_thread_get_frame_at, (int32_t offset, Il2CppStackFrameInfo* frame))
DO_API(bool,              il2cpp_thread_get_frame_at,   (Il2CppThread* thread, int32_t offset, Il2CppStackFrameInfo* frame))
DO_API(int32_t,           il2cpp_current_thread_get_stack_depth, ())
DO_API(int32_t,           il2cpp_thread_get_stack_depth,(Il2CppThread* thread))
DO_API(void,              il2cpp_override_stack_backtrace, (Il2CppBacktraceFunc stackBacktraceFunc))
DO_API(Il2CppObject*,     il2cpp_type_get_object,       (const Il2CppType* type))
DO_API(int,               il2cpp_type_get_type,         (const Il2CppType* type))
DO_API(Il2CppClass*,      il2cpp_type_get_class_or_element_class, (const Il2CppType* type))
DO_API(char*,             il2cpp_type_get_name,         (const Il2CppType* type))
DO_API(bool,              il2cpp_type_is_byref,         (const Il2CppType* type))
DO_API(uint32_t,          il2cpp_type_get_attrs,        (const Il2CppType* type))
DO_API(bool,              il2cpp_type_equals,           (const Il2CppType* type, const Il2CppType* otherType))
DO_API(char*,             il2cpp_type_get_assembly_qualified_name, (const Il2CppType* type))
DO_API(bool,              il2cpp_type_is_static,        (const Il2CppType* type))
DO_API(bool,              il2cpp_type_is_pointer_type,  (const Il2CppType* type))
DO_API(const Il2CppAssembly*, il2cpp_image_get_assembly, (const Il2CppImage* image))
DO_API(const char*,       il2cpp_image_get_name,        (const Il2CppImage* image))
DO_API(const char*,       il2cpp_image_get_filename,    (const Il2CppImage* image))
DO_API(const MethodInfo*, il2cpp_image_get_entry_point, (const Il2CppImage* image))
DO_API(size_t,            il2cpp_image_get_class_count, (const Il2CppImage* image))
DO_API(const Il2CppClass*, il2cpp_image_get_class,      (const Il2CppImage* image, size_t index))
DO_API(Il2CppManagedMemorySnapshot*, il2cpp_capture_memory_snapshot, ())
DO_API(void,              il2cpp_free_captured_memory_snapshot, (Il2CppManagedMemorySnapshot* snapshot))
DO_API(void,              il2cpp_set_find_plugin_callback, (Il2CppSetFindPlugInCallback method))
DO_API(void,              il2cpp_register_log_callback, (Il2CppLogCallback method))
DO_API(void,              il2cpp_debugger_set_agent_options, (const char* options))
DO_API(bool,              il2cpp_is_debugger_attached,  ())
DO_API(void,              il2cpp_register_debugger_agent_transport, (Il2CppDebuggerTransport* debuggerTransport))
DO_API(bool,              il2cpp_debug_get_method_info, (const MethodInfo*, Il2CppMethodDebugInfo* methodDebugInfo))
DO_API(void,              il2cpp_unity_install_unitytls_interface, (const void* unitytlsInterfaceStruct))
DO_API(Il2CppCustomAttrInfo*, il2cpp_custom_attrs_from_class, (Il2CppClass* klass))
DO_API(Il2CppCustomAttrInfo*, il2cpp_custom_attrs_from_method, (const MethodInfo* method))
DO_API(Il2CppObject*,     il2cpp_custom_attrs_get_attr, (Il2CppCustomAttrInfo* ainfo, Il2CppClass* attr_klass))
DO_API(bool,              il2cpp_custom_attrs_has_attr,  (Il2CppCustomAttrInfo* ainfo, Il2CppClass* attr_klass))
DO_API(Il2CppArray*,      il2cpp_custom_attrs_construct, (Il2CppCustomAttrInfo* cinfo))
DO_API(void,              il2cpp_custom_attrs_free,     (Il2CppCustomAttrInfo* ainfo))
DO_API(void,              il2cpp_class_set_userdata,    (Il2CppClass* klass, void* userdata))
DO_API(int,               il2cpp_class_get_userdata_offset, ())
DO_API(void,              il2cpp_set_default_thread_affinity, (int64_t affinity_mask))

#endif

namespace scanner {

struct PatternByte { uint8_t value; bool is_wildcard; };

template<size_t N>
using Pattern = std::array<PatternByte, N>;

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
}

struct MappedRegion { uintptr_t base; size_t size; };

struct ResolvedSymbols {
    uintptr_t domain_get            = 0;
    uintptr_t domain_get_assemblies = 0;
    uintptr_t image_get_class       = 0;
    uintptr_t thread_attach         = 0;
    uintptr_t is_vm_thread          = 0;
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
    if (!region) { LOGE("scanner: failed to resolve region at 0x%" PRIxPTR, library_base); return {}; }
    ResolvedSymbols s;
    auto try_scan = [&](auto& field, const auto& pat, const char* name) {
        if (auto a = scanPattern(*region, pat)) {
            field = *a; LOGI("scanner: %s -> 0x%" PRIxPTR, name, *a);
        } else { LOGW("scanner: %s pattern not found", name); }
    };
    try_scan(s.domain_get,            arm64_prologue::kDomainGet,           "il2cpp_domain_get");
    try_scan(s.domain_get_assemblies, arm64_prologue::kDomainGetAssemblies, "il2cpp_domain_get_assemblies");
    try_scan(s.image_get_class,       arm64_prologue::kImageGetClass,       "il2cpp_image_get_class");
    try_scan(s.thread_attach,         arm64_prologue::kThreadAttach,        "il2cpp_thread_attach");
    try_scan(s.is_vm_thread,          arm64_prologue::kIsVmThread,          "il2cpp_is_vm_thread");
    return s;
}

}

namespace config {

inline constexpr std::string_view kConfigPath     = "/data/local/tmp/eaquel_config.json";
inline constexpr std::string_view kFallbackOutput = "/sdcard/Eaquel";

struct DumperConfig {
    std::string target_package;
    std::string output_dir;
    bool        generate_cpp_header   = true;
    bool        generate_frida_script = true;
};

[[nodiscard]] static std::optional<std::string> extractJsonString(
    const std::string& json, std::string_view key
) {
    auto search = std::string("\"") + std::string(key) + "\"";
    auto kp     = json.find(search);
    if (kp == std::string::npos) return std::nullopt;
    auto cp = json.find(':', kp + search.size());
    if (cp == std::string::npos) return std::nullopt;
    auto qo = json.find('"', cp + 1);
    if (qo == std::string::npos) return std::nullopt;
    auto qc = json.find('"', qo + 1);
    if (qc == std::string::npos) return std::nullopt;
    return json.substr(qo + 1, qc - qo - 1);
}

[[nodiscard]] static bool extractJsonBool(
    const std::string& json, std::string_view key, bool def
) {
    auto search = std::string("\"") + std::string(key) + "\"";
    auto kp     = json.find(search);
    if (kp == std::string::npos) return def;
    auto cp = json.find(':', kp + search.size());
    if (cp == std::string::npos) return def;
    auto vs = json.find_first_not_of(" \t\n\r", cp + 1);
    if (vs == std::string::npos) return def;
    return json.substr(vs, 4) == "true";
}

[[nodiscard]] static DumperConfig loadConfig() {
    DumperConfig cfg;
    cfg.output_dir = std::string(kFallbackOutput);
    std::ifstream file(kConfigPath.data());
    if (!file.is_open()) {
        LOGW("config: %s not found, using defaults", kConfigPath.data());
        return cfg;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    auto json = buf.str();
    if (auto v = extractJsonString(json, "target_package")) {
        cfg.target_package = *v;
        LOGI("config: target_package = %s", cfg.target_package.c_str());
    } else { LOGW("config: target_package missing"); }
    if (auto v = extractJsonString(json, "output_dir")) {
        cfg.output_dir = *v;
        LOGI("config: output_dir = %s", cfg.output_dir.c_str());
    }
    cfg.generate_cpp_header   = extractJsonBool(json, "generate_cpp_header",   true);
    cfg.generate_frida_script = extractJsonBool(json, "generate_frida_script", true);
    LOGI("config: cpp_header=%d frida_script=%d", cfg.generate_cpp_header, cfg.generate_frida_script);
    return cfg;
}

[[nodiscard]] static bool isTargetPackage(const DumperConfig& cfg, std::string_view pkg) {
    return !cfg.target_package.empty() && cfg.target_package == pkg;
}

}
