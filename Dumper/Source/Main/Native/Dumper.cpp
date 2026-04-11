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

#define DO_API(r, n, p) r (*n) p;

#include "Core.h"

#undef DO_API

static uint64_t il2cpp_base = 0;

static void init_il2cpp_api(void *handle) {
#define DO_API(r, n, p)                              \
    {                                                \
        n = reinterpret_cast<r (*) p>(               \
            xdl_sym(handle, #n, nullptr));           \
        if (!n) LOGW("symbol not resolved: %s", #n); \
    }

#include "Core.h"

#undef DO_API
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
    if (flags & METHOD_ATTRIBUTE_STATIC) out << "static ";
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

static bool resolve_byref(const Il2CppType *type) {
    if (il2cpp_type_is_byref) return il2cpp_type_is_byref(type);
    return static_cast<bool>(type->byref);
}

static std::string dump_methods(Il2CppClass *klass) {
    std::stringstream out;
    out << "\n\t// Methods\n";
    void *iter = nullptr;
    while (auto method = il2cpp_class_get_methods(klass, &iter)) {
        if (method->methodPointer) {
            out << "\t// RVA: 0x" << std::hex
                << (reinterpret_cast<uint64_t>(method->methodPointer) - il2cpp_base)
                << " VA: 0x" << std::hex
                << reinterpret_cast<uint64_t>(method->methodPointer);
        } else {
            out << "\t// RVA: 0x0 VA: 0x0";
        }
        out << "\n\t";
        uint32_t iflags = 0;
        out << get_method_modifier(il2cpp_method_get_flags(method, &iflags));
        auto return_type = il2cpp_method_get_return_type(method);
        if (resolve_byref(return_type)) out << "ref ";
        out << il2cpp_class_get_name(il2cpp_class_from_type(return_type))
            << " " << il2cpp_method_get_name(method) << "(";
        auto param_count = il2cpp_method_get_param_count(method);
        for (uint32_t i = 0; i < param_count; ++i) {
            auto param = il2cpp_method_get_param(method, i);
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
            out << il2cpp_class_get_name(il2cpp_class_from_type(param))
                << " " << il2cpp_method_get_param_name(method, i) << ", ";
        }
        if (param_count > 0) out.seekp(-2, out.cur);
        out << ") { }\n";
    }
    return out.str();
}

static std::string dump_properties(Il2CppClass *klass) {
    std::stringstream out;
    out << "\n\t// Properties\n";
    void *iter = nullptr;
    while (auto prop_const = il2cpp_class_get_properties(klass, &iter)) {
        auto prop = const_cast<PropertyInfo *>(prop_const);
        auto get_m = il2cpp_property_get_get_method(prop);
        auto set_m = il2cpp_property_get_set_method(prop);
        auto prop_name = il2cpp_property_get_name(prop);
        out << "\t";
        Il2CppClass *prop_class = nullptr;
        uint32_t iflags = 0;
        if (get_m) {
            out << get_method_modifier(il2cpp_method_get_flags(get_m, &iflags));
            prop_class = il2cpp_class_from_type(il2cpp_method_get_return_type(get_m));
        } else if (set_m) {
            out << get_method_modifier(il2cpp_method_get_flags(set_m, &iflags));
            prop_class = il2cpp_class_from_type(il2cpp_method_get_param(set_m, 0));
        }
        if (prop_class) {
            out << il2cpp_class_get_name(prop_class) << " " << prop_name << " { ";
            if (get_m) out << "get; ";
            if (set_m) out << "set; ";
            out << "}\n";
        } else if (prop_name) {
            out << " // unknown property " << prop_name << "\n";
        }
    }
    return out.str();
}

static std::string dump_fields(Il2CppClass *klass) {
    std::stringstream out;
    out << "\n\t// Fields\n";
    auto is_enum = il2cpp_class_is_enum(klass);
    void *iter = nullptr;
    while (auto field = il2cpp_class_get_fields(klass, &iter)) {
        out << "\t";
        auto attrs = il2cpp_field_get_flags(field);
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
        auto field_class = il2cpp_class_from_type(il2cpp_field_get_type(field));
        out << il2cpp_class_get_name(field_class) << " " << il2cpp_field_get_name(field);
        if ((attrs & FIELD_ATTRIBUTE_LITERAL) && is_enum) {
            uint64_t val = 0;
            il2cpp_field_static_get_value(field, &val);
            out << " = " << std::dec << val;
        }
        out << "; // 0x" << std::hex << il2cpp_field_get_offset(field) << "\n";
    }
    return out.str();
}

static std::string dump_type(const Il2CppType *type) {
    std::stringstream out;
    auto klass = il2cpp_class_from_type(type);
    out << "\n// Namespace: " << il2cpp_class_get_namespace(klass) << "\n";
    auto flags = il2cpp_class_get_flags(klass);
    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE) out << "[Serializable]\n";
    auto is_valuetype = il2cpp_class_is_valuetype(klass);
    auto is_enum = il2cpp_class_is_enum(klass);
    switch (flags & TYPE_ATTRIBUTE_VISIBILITY_MASK) {
        case TYPE_ATTRIBUTE_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_PUBLIC:      out << "public ";            break;
        case TYPE_ATTRIBUTE_NOT_PUBLIC:
        case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
        case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:    out << "internal ";          break;
        case TYPE_ATTRIBUTE_NESTED_PRIVATE:     out << "private ";           break;
        case TYPE_ATTRIBUTE_NESTED_FAMILY:      out << "protected ";         break;
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
    out << il2cpp_class_get_name(klass);

    std::vector<std::string> extends;
    auto parent = il2cpp_class_get_parent(klass);
    if (!is_valuetype && !is_enum && parent) {
        auto parent_type = il2cpp_class_get_type(parent);
        if (parent_type->type != IL2CPP_TYPE_OBJECT)
            extends.emplace_back(il2cpp_class_get_name(parent));
    }
    void *iter = nullptr;
    while (auto itf = il2cpp_class_get_interfaces(klass, &iter))
        extends.emplace_back(il2cpp_class_get_name(itf));
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

void il2cpp_api_init(void *handle) {
    LOGI("il2cpp handle: %p", handle);
    init_il2cpp_api(handle);
    if (!il2cpp_domain_get_assemblies) {
        LOGE("il2cpp api init failed");
        return;
    }
    Dl_info dl_info{};
    if (dladdr(reinterpret_cast<void *>(il2cpp_domain_get_assemblies), &dl_info)) {
        il2cpp_base = reinterpret_cast<uint64_t>(dl_info.dli_fbase);
    }
    LOGI("il2cpp_base: 0x%" PRIx64, il2cpp_base);
    while (!il2cpp_is_vm_thread(nullptr)) {
        LOGI("waiting for il2cpp init...");
        sleep(1);
    }
    auto domain = il2cpp_domain_get();
    il2cpp_thread_attach(domain);
}

void il2cpp_dump(const char *out_dir) {
    LOGI("dump started");
    size_t assembly_count = 0;
    auto domain = il2cpp_domain_get();
    auto assemblies = il2cpp_domain_get_assemblies(domain, &assembly_count);

    std::stringstream image_header;
    for (size_t i = 0; i < assembly_count; ++i) {
        auto image = il2cpp_assembly_get_image(assemblies[i]);
        image_header << "// Image " << i << ": " << il2cpp_image_get_name(image) << "\n";
    }

    std::vector<std::string> type_outputs;

    if (il2cpp_image_get_class) {
        LOGI("il2cpp >= 2018.3 path");
        for (size_t i = 0; i < assembly_count; ++i) {
            auto image = il2cpp_assembly_get_image(assemblies[i]);
            std::string dll_header = std::string("\n// Dll : ") + il2cpp_image_get_name(image);
            auto class_count = il2cpp_image_get_class_count(image);
            for (size_t j = 0; j < class_count; ++j) {
                auto klass = il2cpp_image_get_class(image, j);
                auto type = il2cpp_class_get_type(const_cast<Il2CppClass *>(klass));
                type_outputs.push_back(dll_header + dump_type(type));
            }
        }
    } else {
        LOGI("il2cpp < 2018.3 reflection path");
        auto corlib = il2cpp_get_corlib();
        auto assembly_class = il2cpp_class_from_name(corlib, "System.Reflection", "Assembly");
        auto assembly_load = il2cpp_class_get_method_from_name(assembly_class, "Load", 1);
        auto assembly_get_types = il2cpp_class_get_method_from_name(assembly_class, "GetTypes", 0);
        if (!assembly_load || !assembly_load->methodPointer) {
            LOGE("Assembly::Load not found");
            return;
        }
        if (!assembly_get_types || !assembly_get_types->methodPointer) {
            LOGE("Assembly::GetTypes not found");
            return;
        }
        using Assembly_Load_fn = void *(*)(void *, Il2CppString *, void *);
        using Assembly_GetTypes_fn = Il2CppArray *(*)(void *, void *);
        for (size_t i = 0; i < assembly_count; ++i) {
            auto image = il2cpp_assembly_get_image(assemblies[i]);
            std::string dll_header = std::string("\n// Dll : ") + il2cpp_image_get_name(image);
            auto image_name = std::string(il2cpp_image_get_name(image));
            auto dot_pos = image_name.rfind('.');
            auto name_no_ext = image_name.substr(0, dot_pos);
            auto assembly_name_str = il2cpp_string_new(name_no_ext.data());
            auto reflection_assembly = reinterpret_cast<Assembly_Load_fn>(
                assembly_load->methodPointer)(nullptr, assembly_name_str, nullptr);
            auto reflection_types = reinterpret_cast<Assembly_GetTypes_fn>(
                assembly_get_types->methodPointer)(reflection_assembly, nullptr);
            for (size_t j = 0; j < reflection_types->max_length; ++j) {
                auto klass = il2cpp_class_from_system_type(
                    static_cast<Il2CppReflectionType *>(reflection_types->vector[j]));
                type_outputs.push_back(dll_header + dump_type(il2cpp_class_get_type(klass)));
            }
        }
    }

    auto out_path = std::string(out_dir) + "/files/dump.cs";
    std::ofstream out_stream(out_path);
    out_stream << image_header.str();
    for (const auto &entry : type_outputs)
        out_stream << entry;
    out_stream.close();
    LOGI("dump written to %s", out_path.c_str());
}
