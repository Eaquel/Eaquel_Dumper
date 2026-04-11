#include "Core.h"
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <xdl.h>

namespace il2cpp_api {

struct FunctionTable {
    int                   (*init)(const char*) = nullptr;
    Il2CppDomain*         (*domain_get)() = nullptr;
    const Il2CppAssembly** (*domain_get_assemblies)(const Il2CppDomain*, size_t*) = nullptr;
    const Il2CppImage*    (*assembly_get_image)(const Il2CppAssembly*) = nullptr;
    const char*           (*image_get_name)(const Il2CppImage*) = nullptr;
    size_t                (*image_get_class_count)(const Il2CppImage*) = nullptr;
    const Il2CppClass*    (*image_get_class)(const Il2CppImage*, size_t) = nullptr;
    const Il2CppType*     (*class_get_type)(Il2CppClass*) = nullptr;
    Il2CppClass*          (*class_from_type)(const Il2CppType*) = nullptr;
    const char*           (*class_get_name)(Il2CppClass*) = nullptr;
    const char*           (*class_get_namespace)(Il2CppClass*) = nullptr;
    int                   (*class_get_flags)(const Il2CppClass*) = nullptr;
    bool                  (*class_is_valuetype)(const Il2CppClass*) = nullptr;
    bool                  (*class_is_enum)(const Il2CppClass*) = nullptr;
    bool                  (*class_is_abstract)(const Il2CppClass*) = nullptr;
    bool                  (*class_is_interface)(const Il2CppClass*) = nullptr;
    Il2CppClass*          (*class_get_parent)(Il2CppClass*) = nullptr;
    Il2CppClass*          (*class_get_interfaces)(Il2CppClass*, void**) = nullptr;
    FieldInfo*            (*class_get_fields)(Il2CppClass*, void**) = nullptr;
    const PropertyInfo*   (*class_get_properties)(Il2CppClass*, void**) = nullptr;
    const MethodInfo*     (*class_get_methods)(Il2CppClass*, void**) = nullptr;
    const MethodInfo*     (*class_get_method_from_name)(const Il2CppClass*, const char*, int) = nullptr;
    Il2CppClass*          (*class_from_name)(const Il2CppImage*, const char*, const char*) = nullptr;
    Il2CppClass*          (*class_from_system_type)(Il2CppReflectionType*) = nullptr;
    const Il2CppImage*    (*get_corlib)() = nullptr;
    int                   (*field_get_flags)(FieldInfo*) = nullptr;
    const char*           (*field_get_name)(FieldInfo*) = nullptr;
    size_t                (*field_get_offset)(FieldInfo*) = nullptr;
    const Il2CppType*     (*field_get_type)(FieldInfo*) = nullptr;
    void                  (*field_static_get_value)(FieldInfo*, void*) = nullptr;
    uint32_t              (*property_get_flags)(PropertyInfo*) = nullptr;
    const MethodInfo*     (*property_get_get_method)(PropertyInfo*) = nullptr;
    const MethodInfo*     (*property_get_set_method)(PropertyInfo*) = nullptr;
    const char*           (*property_get_name)(PropertyInfo*) = nullptr;
    const Il2CppType*     (*method_get_return_type)(const MethodInfo*) = nullptr;
    const char*           (*method_get_name)(const MethodInfo*) = nullptr;
    uint32_t              (*method_get_param_count)(const MethodInfo*) = nullptr;
    const Il2CppType*     (*method_get_param)(const MethodInfo*, uint32_t) = nullptr;
    uint32_t              (*method_get_flags)(const MethodInfo*, uint32_t*) = nullptr;
    const char*           (*method_get_param_name)(const MethodInfo*, uint32_t) = nullptr;
    bool                  (*type_is_byref)(const Il2CppType*) = nullptr;
    Il2CppThread*         (*thread_attach)(Il2CppDomain*) = nullptr;
    bool                  (*is_vm_thread)(Il2CppThread*) = nullptr;
    Il2CppString*         (*string_new)(const char*) = nullptr;
};

static FunctionTable g_api;
static uint64_t      g_il2cpp_base = 0;

template<typename T>
static void resolve_sym(void* handle, const char* name, T& out) {
    out = reinterpret_cast<T>(xdl_sym(handle, name, nullptr));
    if (!out) LOGW("symbol not resolved: %s", name);
}

static void populate_table(void* handle) {
    resolve_sym(handle, "il2cpp_init",                       g_api.init);
    resolve_sym(handle, "il2cpp_domain_get",                 g_api.domain_get);
    resolve_sym(handle, "il2cpp_domain_get_assemblies",      g_api.domain_get_assemblies);
    resolve_sym(handle, "il2cpp_assembly_get_image",         g_api.assembly_get_image);
    resolve_sym(handle, "il2cpp_image_get_name",             g_api.image_get_name);
    resolve_sym(handle, "il2cpp_image_get_class_count",      g_api.image_get_class_count);
    resolve_sym(handle, "il2cpp_image_get_class",            g_api.image_get_class);
    resolve_sym(handle, "il2cpp_class_get_type",             g_api.class_get_type);
    resolve_sym(handle, "il2cpp_class_from_type",            g_api.class_from_type);
    resolve_sym(handle, "il2cpp_class_get_name",             g_api.class_get_name);
    resolve_sym(handle, "il2cpp_class_get_namespace",        g_api.class_get_namespace);
    resolve_sym(handle, "il2cpp_class_get_flags",            g_api.class_get_flags);
    resolve_sym(handle, "il2cpp_class_is_valuetype",         g_api.class_is_valuetype);
    resolve_sym(handle, "il2cpp_class_is_enum",              g_api.class_is_enum);
    resolve_sym(handle, "il2cpp_class_is_abstract",          g_api.class_is_abstract);
    resolve_sym(handle, "il2cpp_class_is_interface",         g_api.class_is_interface);
    resolve_sym(handle, "il2cpp_class_get_parent",           g_api.class_get_parent);
    resolve_sym(handle, "il2cpp_class_get_interfaces",       g_api.class_get_interfaces);
    resolve_sym(handle, "il2cpp_class_get_fields",           g_api.class_get_fields);
    resolve_sym(handle, "il2cpp_class_get_properties",       g_api.class_get_properties);
    resolve_sym(handle, "il2cpp_class_get_methods",          g_api.class_get_methods);
    resolve_sym(handle, "il2cpp_class_get_method_from_name", g_api.class_get_method_from_name);
    resolve_sym(handle, "il2cpp_class_from_name",            g_api.class_from_name);
    resolve_sym(handle, "il2cpp_class_from_system_type",     g_api.class_from_system_type);
    resolve_sym(handle, "il2cpp_get_corlib",                 g_api.get_corlib);
    resolve_sym(handle, "il2cpp_field_get_flags",            g_api.field_get_flags);
    resolve_sym(handle, "il2cpp_field_get_name",             g_api.field_get_name);
    resolve_sym(handle, "il2cpp_field_get_offset",           g_api.field_get_offset);
    resolve_sym(handle, "il2cpp_field_get_type",             g_api.field_get_type);
    resolve_sym(handle, "il2cpp_field_static_get_value",     g_api.field_static_get_value);
    resolve_sym(handle, "il2cpp_property_get_flags",         g_api.property_get_flags);
    resolve_sym(handle, "il2cpp_property_get_get_method",    g_api.property_get_get_method);
    resolve_sym(handle, "il2cpp_property_get_set_method",    g_api.property_get_set_method);
    resolve_sym(handle, "il2cpp_property_get_name",          g_api.property_get_name);
    resolve_sym(handle, "il2cpp_method_get_return_type",     g_api.method_get_return_type);
    resolve_sym(handle, "il2cpp_method_get_name",            g_api.method_get_name);
    resolve_sym(handle, "il2cpp_method_get_param_count",     g_api.method_get_param_count);
    resolve_sym(handle, "il2cpp_method_get_param",           g_api.method_get_param);
    resolve_sym(handle, "il2cpp_method_get_flags",           g_api.method_get_flags);
    resolve_sym(handle, "il2cpp_method_get_param_name",      g_api.method_get_param_name);
    resolve_sym(handle, "il2cpp_type_is_byref",              g_api.type_is_byref);
    resolve_sym(handle, "il2cpp_thread_attach",              g_api.thread_attach);
    resolve_sym(handle, "il2cpp_is_vm_thread",               g_api.is_vm_thread);
    resolve_sym(handle, "il2cpp_string_new",                 g_api.string_new);
}

}

static std::string get_method_modifier(uint32_t flags) {
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

static bool resolve_byref(const Il2CppType* type) {
    auto& api = il2cpp_api::g_api;
    if (api.type_is_byref) return api.type_is_byref(type);
    return static_cast<bool>(type->byref);
}

static std::string dump_methods(Il2CppClass* klass) {
    auto& api = il2cpp_api::g_api;
    std::stringstream out;
    out << "\n\t// Methods\n";
    void* iter = nullptr;
    while (auto method = api.class_get_methods(klass, &iter)) {
        if (method->methodPointer) {
            out << "\t// RVA: 0x" << std::hex
                << (reinterpret_cast<uint64_t>(method->methodPointer) - il2cpp_api::g_il2cpp_base)
                << " VA: 0x" << std::hex
                << reinterpret_cast<uint64_t>(method->methodPointer);
        } else {
            out << "\t// RVA: 0x0 VA: 0x0";
        }
        out << "\n\t";
        uint32_t iflags = 0;
        out << get_method_modifier(api.method_get_flags(method, &iflags));
        auto return_type = api.method_get_return_type(method);
        if (resolve_byref(return_type)) out << "ref ";
        out << api.class_get_name(api.class_from_type(return_type))
            << " " << api.method_get_name(method) << "(";
        auto param_count = api.method_get_param_count(method);
        for (uint32_t i = 0; i < param_count; ++i) {
            auto param = api.method_get_param(method, i);
            auto attrs = param->attrs;
            if (resolve_byref(param)) {
                if ((attrs & PARAM_ATTRIBUTE_OUT) && !(attrs & PARAM_ATTRIBUTE_IN))
                    out << "out ";
                else if ((attrs & PARAM_ATTRIBUTE_IN) && !(attrs & PARAM_ATTRIBUTE_OUT))
                    out << "in ";
                else
                    out << "ref ";
            } else {
                if (attrs & PARAM_ATTRIBUTE_IN)  out << "[In] ";
                if (attrs & PARAM_ATTRIBUTE_OUT) out << "[Out] ";
            }
            out << api.class_get_name(api.class_from_type(param))
                << " " << api.method_get_param_name(method, i) << ", ";
        }
        if (param_count > 0) out.seekp(-2, out.cur);
        out << ") { }\n";
    }
    return out.str();
}

static std::string dump_properties(Il2CppClass* klass) {
    auto& api = il2cpp_api::g_api;
    std::stringstream out;
    out << "\n\t// Properties\n";
    void* iter = nullptr;
    while (auto prop_const = api.class_get_properties(klass, &iter)) {
        auto prop = const_cast<PropertyInfo*>(prop_const);
        auto get_m = api.property_get_get_method(prop);
        auto set_m = api.property_get_set_method(prop);
        auto prop_name = api.property_get_name(prop);
        out << "\t";
        Il2CppClass* prop_class = nullptr;
        uint32_t iflags = 0;
        if (get_m) {
            out << get_method_modifier(api.method_get_flags(get_m, &iflags));
            prop_class = api.class_from_type(api.method_get_return_type(get_m));
        } else if (set_m) {
            out << get_method_modifier(api.method_get_flags(set_m, &iflags));
            prop_class = api.class_from_type(api.method_get_param(set_m, 0));
        }
        if (prop_class) {
            out << api.class_get_name(prop_class) << " " << prop_name << " { ";
            if (get_m) out << "get; ";
            if (set_m) out << "set; ";
            out << "}\n";
        } else if (prop_name) {
            out << " // unknown property " << prop_name << "\n";
        }
    }
    return out.str();
}

static std::string dump_fields(Il2CppClass* klass) {
    auto& api = il2cpp_api::g_api;
    std::stringstream out;
    out << "\n\t// Fields\n";
    auto is_enum = api.class_is_enum(klass);
    void* iter = nullptr;
    while (auto field = api.class_get_fields(klass, &iter)) {
        out << "\t";
        auto attrs = api.field_get_flags(field);
        switch (attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK) {
            case FIELD_ATTRIBUTE_PRIVATE:       out << "private ";            break;
            case FIELD_ATTRIBUTE_PUBLIC:        out << "public ";             break;
            case FIELD_ATTRIBUTE_FAMILY:        out << "protected ";          break;
            case FIELD_ATTRIBUTE_ASSEMBLY:
            case FIELD_ATTRIBUTE_FAM_AND_ASSEM: out << "internal ";           break;
            case FIELD_ATTRIBUTE_FAM_OR_ASSEM:  out << "protected internal "; break;
            default: break;
        }
        if (attrs & FIELD_ATTRIBUTE_LITERAL) {
            out << "const ";
        } else {
            if (attrs & FIELD_ATTRIBUTE_STATIC)    out << "static ";
            if (attrs & FIELD_ATTRIBUTE_INIT_ONLY) out << "readonly ";
        }
        auto field_class = api.class_from_type(api.field_get_type(field));
        out << api.class_get_name(field_class) << " " << api.field_get_name(field);
        if ((attrs & FIELD_ATTRIBUTE_LITERAL) && is_enum) {
            uint64_t val = 0;
            api.field_static_get_value(field, &val);
            out << " = " << std::dec << val;
        }
        out << "; // 0x" << std::hex << api.field_get_offset(field) << "\n";
    }
    return out.str();
}

static std::string dump_type(const Il2CppType* type) {
    auto& api = il2cpp_api::g_api;
    std::stringstream out;
    auto klass = api.class_from_type(type);
    out << "\n// Namespace: " << api.class_get_namespace(klass) << "\n";
    auto flags = api.class_get_flags(klass);
    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE) out << "[Serializable]\n";
    auto is_valuetype = api.class_is_valuetype(klass);
    auto is_enum = api.class_is_enum(klass);
    switch (flags & TYPE_ATTRIBUTE_VISIBILITY_MASK) {
        case TYPE_ATTRIBUTE_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_PUBLIC:       out << "public ";             break;
        case TYPE_ATTRIBUTE_NOT_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
        case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:     out << "internal ";           break;
        case TYPE_ATTRIBUTE_NESTED_PRIVATE:      out << "private ";            break;
        case TYPE_ATTRIBUTE_NESTED_FAMILY:       out << "protected ";          break;
        case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM: out << "protected internal "; break;
        default: break;
    }
    if ((flags & TYPE_ATTRIBUTE_ABSTRACT) && (flags & TYPE_ATTRIBUTE_SEALED)) {
        out << "static ";
    } else if (!(flags & TYPE_ATTRIBUTE_INTERFACE) && (flags & TYPE_ATTRIBUTE_ABSTRACT)) {
        out << "abstract ";
    } else if (!is_valuetype && !is_enum && (flags & TYPE_ATTRIBUTE_SEALED)) {
        out << "sealed ";
    }
    if (flags & TYPE_ATTRIBUTE_INTERFACE)  out << "interface ";
    else if (is_enum)                      out << "enum ";
    else if (is_valuetype)                 out << "struct ";
    else                                   out << "class ";
    out << api.class_get_name(klass);

    std::vector<std::string> extends;
    auto parent = api.class_get_parent(klass);
    if (!is_valuetype && !is_enum && parent) {
        auto parent_type = api.class_get_type(parent);
        if (parent_type->type != IL2CPP_TYPE_OBJECT)
            extends.emplace_back(api.class_get_name(parent));
    }
    void* iter = nullptr;
    while (auto itf = api.class_get_interfaces(klass, &iter))
        extends.emplace_back(api.class_get_name(itf));
    if (!extends.empty()) {
        out << " : " << extends[0];
        for (size_t i = 1; i < extends.size(); ++i)
            out << ", " << extends[i];
    }
    out << "\n{";
    out << dump_fields(klass);
    out << dump_properties(klass);
    out << dump_methods(klass);
    out << "}\n";
    return out.str();
}

void il2cpp_api_init(void* handle) {
    LOGI("il2cpp handle: %p", handle);
    il2cpp_api::populate_table(handle);

    auto& api = il2cpp_api::g_api;
    if (!api.domain_get_assemblies) {
        LOGE("il2cpp api init failed");
        return;
    }
    Dl_info dl_info{};
    if (dladdr(reinterpret_cast<void*>(api.domain_get_assemblies), &dl_info)) {
        il2cpp_api::g_il2cpp_base = reinterpret_cast<uint64_t>(dl_info.dli_fbase);
    }
    LOGI("il2cpp_base: 0x%" PRIx64, il2cpp_api::g_il2cpp_base);

    while (!api.is_vm_thread(nullptr)) {
        LOGI("waiting for il2cpp init...");
        sleep(1);
    }
    auto domain = api.domain_get();
    api.thread_attach(domain);
}

void il2cpp_dump(const char* out_dir) {
    auto& api = il2cpp_api::g_api;
    LOGI("dump started");
    size_t assembly_count = 0;
    auto domain = api.domain_get();
    auto assemblies = api.domain_get_assemblies(domain, &assembly_count);

    std::stringstream image_header;
    for (size_t i = 0; i < assembly_count; ++i) {
        auto image = api.assembly_get_image(assemblies[i]);
        image_header << "// Image " << i << ": " << api.image_get_name(image) << "\n";
    }

    std::vector<std::string> type_outputs;

    if (api.image_get_class) {
        LOGI("il2cpp >= 2018.3 path");
        for (size_t i = 0; i < assembly_count; ++i) {
            auto image = api.assembly_get_image(assemblies[i]);
            std::string dll_header = std::string("\n// Dll : ") + api.image_get_name(image);
            auto class_count = api.image_get_class_count(image);
            for (size_t j = 0; j < class_count; ++j) {
                auto klass = api.image_get_class(image, j);
                auto type = api.class_get_type(const_cast<Il2CppClass*>(klass));
                type_outputs.push_back(dll_header + dump_type(type));
            }
        }
    } else {
        LOGI("il2cpp < 2018.3 reflection path");
        auto corlib = api.get_corlib();
        auto assembly_class = api.class_from_name(corlib, "System.Reflection", "Assembly");
        auto assembly_load = api.class_get_method_from_name(assembly_class, "Load", 1);
        auto assembly_get_types = api.class_get_method_from_name(assembly_class, "GetTypes", 0);
        if (!assembly_load || !assembly_load->methodPointer) {
            LOGE("Assembly::Load not found");
            return;
        }
        if (!assembly_get_types || !assembly_get_types->methodPointer) {
            LOGE("Assembly::GetTypes not found");
            return;
        }
        using Assembly_Load_fn     = void* (*)(void*, Il2CppString*, void*);
        using Assembly_GetTypes_fn = Il2CppArray* (*)(void*, void*);
        for (size_t i = 0; i < assembly_count; ++i) {
            auto image = api.assembly_get_image(assemblies[i]);
            std::string dll_header = std::string("\n// Dll : ") + api.image_get_name(image);
            auto image_name    = std::string(api.image_get_name(image));
            auto dot_pos       = image_name.rfind('.');
            auto name_no_ext   = image_name.substr(0, dot_pos);
            auto assembly_name_str = api.string_new(name_no_ext.data());
            auto reflection_assembly = reinterpret_cast<Assembly_Load_fn>(
                assembly_load->methodPointer)(nullptr, assembly_name_str, nullptr);
            auto reflection_types = reinterpret_cast<Assembly_GetTypes_fn>(
                assembly_get_types->methodPointer)(reflection_assembly, nullptr);
            for (size_t j = 0; j < reflection_types->max_length; ++j) {
                auto klass = api.class_from_system_type(
                    static_cast<Il2CppReflectionType*>(reflection_types->vector[j]));
                type_outputs.push_back(dll_header + dump_type(api.class_get_type(klass)));
            }
        }
    }

    auto out_path = std::string(out_dir) + "/files/dump.cs";
    std::ofstream out_stream(out_path);
    out_stream << image_header.str();
    for (const auto& entry : type_outputs)
        out_stream << entry;
    out_stream.close();
    LOGI("dump written to %s", out_path.c_str());
}
