#define DO_API(r, n, p) r (*n) p = nullptr;
#define DO_API_NO_RETURN(r, n, p) r (*n) p = nullptr;
#include "Core.h"
#undef DO_API
#undef DO_API_NO_RETURN

#include "Zygisk.hpp"
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
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
#include <linux/unistd.h>
#include <unistd.h>
#include <array>
#include <xdl.h>

static inline const char* safeStr(const char* s, const char* fallback = "") {
    return (s && *s) ? s : fallback;
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
    Il2CppClass*           (*class_get_parent)(Il2CppClass*)                                   = nullptr;
    Il2CppClass*           (*class_get_interfaces)(Il2CppClass*, void**)                       = nullptr;
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
    out = reinterpret_cast<T>(xdl_sym(handle, name, nullptr));
    if (!out) LOGW("symbol not resolved: %s", name);
}

template<typename T>
static void applyFallback(uintptr_t addr, T& out) {
    if (!out && addr && scanner::isReadableAddress(addr)) {
        out = reinterpret_cast<T>(addr);
        LOGI("scanner fallback: 0x%" PRIxPTR, addr);
    }
}

static void populateFunctionTable(void* handle) {
    resolveSymbol(handle, "il2cpp_init",                       g_api.init);
    resolveSymbol(handle, "il2cpp_domain_get",                 g_api.domain_get);
    resolveSymbol(handle, "il2cpp_domain_get_assemblies",      g_api.domain_get_assemblies);
    resolveSymbol(handle, "il2cpp_assembly_get_image",         g_api.assembly_get_image);
    resolveSymbol(handle, "il2cpp_image_get_name",             g_api.image_get_name);
    resolveSymbol(handle, "il2cpp_image_get_class_count",      g_api.image_get_class_count);
    resolveSymbol(handle, "il2cpp_image_get_class",            g_api.image_get_class);
    resolveSymbol(handle, "il2cpp_class_get_type",             g_api.class_get_type);
    resolveSymbol(handle, "il2cpp_class_from_type",            g_api.class_from_type);
    resolveSymbol(handle, "il2cpp_class_get_name",             g_api.class_get_name);
    resolveSymbol(handle, "il2cpp_class_get_namespace",        g_api.class_get_namespace);
    resolveSymbol(handle, "il2cpp_class_get_flags",            g_api.class_get_flags);
    resolveSymbol(handle, "il2cpp_class_is_valuetype",         g_api.class_is_valuetype);
    resolveSymbol(handle, "il2cpp_class_is_enum",              g_api.class_is_enum);
    resolveSymbol(handle, "il2cpp_class_is_abstract",          g_api.class_is_abstract);
    resolveSymbol(handle, "il2cpp_class_is_interface",         g_api.class_is_interface);
    resolveSymbol(handle, "il2cpp_class_get_parent",           g_api.class_get_parent);
    resolveSymbol(handle, "il2cpp_class_get_interfaces",       g_api.class_get_interfaces);
    resolveSymbol(handle, "il2cpp_class_get_fields",           g_api.class_get_fields);
    resolveSymbol(handle, "il2cpp_class_get_properties",       g_api.class_get_properties);
    resolveSymbol(handle, "il2cpp_class_get_methods",          g_api.class_get_methods);
    resolveSymbol(handle, "il2cpp_class_get_method_from_name", g_api.class_get_method_from_name);
    resolveSymbol(handle, "il2cpp_class_from_name",            g_api.class_from_name);
    resolveSymbol(handle, "il2cpp_class_from_system_type",     g_api.class_from_system_type);
    resolveSymbol(handle, "il2cpp_get_corlib",                 g_api.get_corlib);
    resolveSymbol(handle, "il2cpp_field_get_flags",            g_api.field_get_flags);
    resolveSymbol(handle, "il2cpp_field_get_name",             g_api.field_get_name);
    resolveSymbol(handle, "il2cpp_field_get_offset",           g_api.field_get_offset);
    resolveSymbol(handle, "il2cpp_field_get_type",             g_api.field_get_type);
    resolveSymbol(handle, "il2cpp_field_static_get_value",     g_api.field_static_get_value);
    resolveSymbol(handle, "il2cpp_property_get_flags",         g_api.property_get_flags);
    resolveSymbol(handle, "il2cpp_property_get_get_method",    g_api.property_get_get_method);
    resolveSymbol(handle, "il2cpp_property_get_set_method",    g_api.property_get_set_method);
    resolveSymbol(handle, "il2cpp_property_get_name",          g_api.property_get_name);
    resolveSymbol(handle, "il2cpp_method_get_return_type",     g_api.method_get_return_type);
    resolveSymbol(handle, "il2cpp_method_get_name",            g_api.method_get_name);
    resolveSymbol(handle, "il2cpp_method_get_param_count",     g_api.method_get_param_count);
    resolveSymbol(handle, "il2cpp_method_get_param",           g_api.method_get_param);
    resolveSymbol(handle, "il2cpp_method_get_flags",           g_api.method_get_flags);
    resolveSymbol(handle, "il2cpp_method_get_param_name",      g_api.method_get_param_name);
    resolveSymbol(handle, "il2cpp_type_is_byref",              g_api.type_is_byref);
    resolveSymbol(handle, "il2cpp_thread_attach",              g_api.thread_attach);
    resolveSymbol(handle, "il2cpp_is_vm_thread",               g_api.is_vm_thread);
    resolveSymbol(handle, "il2cpp_string_new",                 g_api.string_new);

    if (!g_api.domain_get || !g_api.domain_get_assemblies ||
        !g_api.image_get_class || !g_api.thread_attach || !g_api.is_vm_thread)
    {
        LOGW("critical symbols missing, attempting pattern scan fallback");
        auto s = scanner::scanAllSymbols(g_il2cpp_base);
        applyFallback(s.domain_get,            g_api.domain_get);
        applyFallback(s.domain_get_assemblies, g_api.domain_get_assemblies);
        applyFallback(s.image_get_class,       g_api.image_get_class);
        applyFallback(s.thread_attach,         g_api.thread_attach);
        applyFallback(s.is_vm_thread,          g_api.is_vm_thread);
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

static bool resolveByref(const Il2CppType* type) {
    auto& api = il2cpp_api::g_api;
    return api.type_is_byref ? api.type_is_byref(type) : static_cast<bool>(type->byref);
}

struct MethodEntry {
    std::string name;
    std::string return_type;
    std::string modifier;
    uint64_t    rva = 0;
    uint64_t    va  = 0;
    std::vector<std::pair<std::string, std::string>> params;
};

static std::string safeClassName(Il2CppClass* klass) {
    if (!klass) return "object";
    auto& api = il2cpp_api::g_api;
    auto name = api.class_get_name ? api.class_get_name(klass) : nullptr;
    return safeStr(name, "object");
}

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
        if (api.method_get_flags)
            e.modifier = buildMethodModifier(api.method_get_flags(method, &iflags));
        if (api.method_get_name)
            e.name = safeStr(api.method_get_name(method));
        if (e.name.empty()) continue;

        if (api.method_get_return_type && api.class_from_type) {
            auto ret = api.method_get_return_type(method);
            if (ret) e.return_type = safeClassName(api.class_from_type(ret));
        }
        if (e.return_type.empty()) e.return_type = "void";

        auto pc = (api.method_get_param_count) ? api.method_get_param_count(method) : 0u;
        for (uint32_t i = 0; i < pc; ++i) {
            std::string tname = "object";
            std::string pname = "p" + std::to_string(i);
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
        if (e.va) {
            out << "\t// RVA: 0x" << std::hex << e.rva << " VA: 0x" << std::hex << e.va;
        } else {
            out << "\t// RVA: 0x0 VA: 0x0";
        }
        out << "\n\t" << e.modifier << e.return_type << " " << e.name << "(";
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
    while (auto pc = api.class_get_properties(klass, &iter)) {
        auto prop  = const_cast<PropertyInfo*>(pc);
        if (!prop) continue;
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
        if (prop_class) {
            out << safeClassName(prop_class) << " " << pname << " { ";
            if (get_m) out << "get; ";
            if (set_m) out << "set; ";
            out << "}\n";
        } else {
            out << " // unknown property " << pname << "\n";
        }
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
        if (attrs & FIELD_ATTRIBUTE_LITERAL) {
            out << "const ";
        } else {
            if (attrs & FIELD_ATTRIBUTE_STATIC)    out << "static ";
            if (attrs & FIELD_ATTRIBUTE_INIT_ONLY) out << "readonly ";
        }
        std::string fname = "unknown_field";
        std::string ftype = "object";
        if (api.field_get_name) { auto n = api.field_get_name(field); if (n && *n) fname = n; }
        if (api.field_get_type && api.class_from_type) {
            auto ft = api.field_get_type(field);
            if (ft) ftype = safeClassName(api.class_from_type(ft));
        }
        out << ftype << " " << fname;
        if ((attrs & FIELD_ATTRIBUTE_LITERAL) && is_enum && api.field_static_get_value) {
            uint64_t val = 0;
            api.field_static_get_value(field, &val);
            out << " = " << std::dec << val;
        }
        size_t offset = api.field_get_offset ? api.field_get_offset(field) : 0;
        out << "; // 0x" << std::hex << offset << "\n";
    }
    return out.str();
}

static std::string dumpType(const Il2CppType* type) {
    if (!type) return {};
    auto& api = il2cpp_api::g_api;
    if (!api.class_from_type) return {};
    auto klass = api.class_from_type(type);
    if (!klass) return {};

    auto  flags       = api.class_get_flags      ? api.class_get_flags(klass)      : 0;
    auto  is_vt       = api.class_is_valuetype   ? api.class_is_valuetype(klass)   : false;
    auto  is_enum     = api.class_is_enum        ? api.class_is_enum(klass)        : false;
    auto  is_iface    = api.class_is_interface   ? api.class_is_interface(klass)   : false;
    auto  is_abstract = api.class_is_abstract    ? api.class_is_abstract(klass)    : false;
    auto  ns          = api.class_get_namespace  ? safeStr(api.class_get_namespace(klass)) : "";
    auto  cn          = api.class_get_name       ? safeStr(api.class_get_name(klass), "UnknownClass") : "UnknownClass";

    std::stringstream out;
    out << "\n// Namespace: " << ns << "\n";
    if (flags & TYPE_ATTRIBUTE_SERIALIZABLE) out << "[Serializable]\n";

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

    if ((flags & TYPE_ATTRIBUTE_ABSTRACT) && (flags & TYPE_ATTRIBUTE_SEALED)) {
        out << "static ";
    } else if (!is_iface && is_abstract) {
        out << "abstract ";
    } else if (flags & TYPE_ATTRIBUTE_SEALED) {
        out << "sealed ";
    }

    if (is_enum)       out << "enum ";
    else if (is_iface) out << "interface ";
    else if (is_vt)    out << "struct ";
    else               out << "class ";

    out << cn;

    if (api.class_get_parent) {
        auto parent = api.class_get_parent(klass);
        if (parent) {
            auto pname = safeClassName(parent);
            if (pname != "object" && pname != "ValueType" && pname != "Enum")
                out << " : " << pname;
        }
    }

    out << "\n{\n";
    out << dumpFields(klass);
    out << dumpProperties(klass);
    out << dumpMethods(klass);
    out << "}\n";
    return out.str();
}

namespace output {

static bool ensureDirectory(const char* dir) {
    struct stat st{};
    if (stat(dir, &st) == 0) return S_ISDIR(st.st_mode);
    if (mkdir(dir, 0777) == 0) {
        chmod(dir, 0777);
        LOGI("created output dir: %s", dir);
        return true;
    }
    LOGE("mkdir failed: %s (errno=%d)", dir, errno);
    return false;
}

static void writeCppHeader(
    const char* dir,
    const std::vector<std::pair<Il2CppClass*, std::string>>& classes
) {
    auto path = std::string(dir) + "/dump.h";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("failed to open %s", path.c_str()); return; }
    auto& api = il2cpp_api::g_api;
    f << "// Generated by Eaquel Dumper\n#pragma once\n#include <cstdint>\n\n";
    for (const auto& [klass, dll] : classes) {
        if (!klass) continue;
        auto cn = api.class_get_name ? api.class_get_name(klass) : nullptr;
        if (!cn || !*cn) continue;
        auto ns = api.class_get_namespace ? api.class_get_namespace(klass) : nullptr;
        f << "// " << dll << " | " << safeStr(ns) << "::" << cn << "\n";
        f << "struct " << cn << " {\n";
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
            f << "    // " << m.modifier << m.return_type << " " << m.name << "() RVA=0x"
              << std::hex << m.rva << "\n";
        }
        f << "};\n\n";
    }
    f.close();
    LOGI("dump.h written to %s", path.c_str());
}

static void writeFridaScript(
    const char* dir,
    const std::vector<std::pair<Il2CppClass*, std::string>>& classes
) {
    auto path = std::string(dir) + "/dump.js";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("failed to open %s", path.c_str()); return; }
    f << "\"use strict\";\n\nconst il2cppBase = Module.findBaseAddress(\"libil2cpp.so\");\n\n";
    auto& api = il2cpp_api::g_api;
    for (const auto& [klass, dll] : classes) {
        if (!klass) continue;
        auto cn = api.class_get_name ? api.class_get_name(klass) : nullptr;
        if (!cn || !*cn) continue;
        for (const auto& m : collectMethods(klass)) {
            if (m.rva == 0 || m.name.empty()) continue;
            f << "// " << cn << "::" << m.name << "\n";
            f << "Interceptor.attach(il2cppBase.add(0x" << std::hex << m.rva << "), {\n";
            f << "    onEnter: function(args) {\n";
            for (size_t i = 0; i < m.params.size(); ++i)
                f << "        // args[" << i << "] -> "
                  << m.params[i].first << " " << m.params[i].second << "\n";
            f << "    },\n    onLeave: function(retval) {\n";
            f << "        // retval -> " << m.return_type << "\n";
            f << "    }\n});\n\n";
        }
    }
    f.close();
    LOGI("dump.js written to %s", path.c_str());
}

} // namespace output

static void runApiInit(void* handle) {
    Dl_info di{};
    if (dladdr(handle, &di))
        il2cpp_api::g_il2cpp_base = reinterpret_cast<uint64_t>(di.dli_fbase);
    LOGI("il2cpp handle: %p  base: 0x%" PRIx64, handle, il2cpp_api::g_il2cpp_base);
    il2cpp_api::populateFunctionTable(handle);

    auto& api = il2cpp_api::g_api;
    if (!api.domain_get_assemblies) { LOGE("il2cpp api init failed — aborting"); return; }

    constexpr int kMaxWaitSec = 30;
    for (int i = 0; i < kMaxWaitSec; ++i) {
        if (!api.is_vm_thread) {
            LOGW("is_vm_thread unavailable, proceeding after delay");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            break;
        }
        if (api.is_vm_thread(nullptr)) break;
        LOGI("waiting for il2cpp VM... (%d/%d)", i + 1, kMaxWaitSec);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (api.thread_attach && api.domain_get) {
        auto domain = api.domain_get();
        if (domain) api.thread_attach(domain);
    }
}

static void runDump(const char* out_dir, const config::DumperConfig& cfg) {
    auto& api = il2cpp_api::g_api;
    if (!api.domain_get || !api.domain_get_assemblies) {
        LOGE("runDump: api not initialised, aborting");
        return;
    }

    LOGI("dump started -> %s", out_dir);

    std::string effective_dir = out_dir;

    auto tryDir = [&](const std::string& d) -> bool {
        if (output::ensureDirectory(d.c_str())) { effective_dir = d; return true; }
        return false;
    };

    if (!tryDir(out_dir)) {
        LOGW("primary output dir unavailable: %s", out_dir);
        if (!tryDir("/data/local/tmp/eaquel_out")) {
            if (!tryDir("/data/local/tmp")) {
                LOGE("runDump: no writable output dir found, aborting");
                return;
            }
        }
    }
    LOGI("runDump: using output dir: %s", effective_dir.c_str());
    const char* dir = effective_dir.c_str();

    size_t ac   = 0;
    auto domain = api.domain_get();
    if (!domain) { LOGE("domain_get returned null"); return; }
    auto assemblies = api.domain_get_assemblies(domain, &ac);
    if (!assemblies || ac == 0) { LOGE("no assemblies found"); return; }

    std::stringstream img_header;
    for (size_t i = 0; i < ac; ++i) {
        if (!assemblies[i]) continue;
        auto img = api.assembly_get_image ? api.assembly_get_image(assemblies[i]) : nullptr;
        if (!img) continue;
        auto iname = api.image_get_name ? api.image_get_name(img) : nullptr;
        img_header << "// Image " << i << ": " << safeStr(iname) << "\n";
    }

    std::vector<std::string>                          type_outputs;
    std::vector<std::pair<Il2CppClass*, std::string>> class_registry;

    if (api.image_get_class) {
        LOGI("il2cpp >= 2018.3 path");
        for (size_t i = 0; i < ac; ++i) {
            if (!assemblies[i]) continue;
            auto img = api.assembly_get_image ? api.assembly_get_image(assemblies[i]) : nullptr;
            if (!img) continue;
            auto iname = api.image_get_name ? api.image_get_name(img) : nullptr;
            auto dll   = std::string(safeStr(iname));
            auto hdr   = "\n// Dll : " + dll;
            auto cc    = api.image_get_class_count ? api.image_get_class_count(img) : 0;
            for (size_t j = 0; j < cc; ++j) {
                auto klass_c = api.image_get_class(img, j);
                if (!klass_c) continue;
                auto klass = const_cast<Il2CppClass*>(klass_c);
                auto type  = api.class_get_type ? api.class_get_type(klass) : nullptr;
                if (!type) continue;
                auto dumped = dumpType(type);
                if (!dumped.empty()) {
                    type_outputs.push_back(hdr + dumped);
                    class_registry.emplace_back(klass, dll);
                }
            }
        }
    } else {
        LOGI("il2cpp < 2018.3 reflection path");
        if (!api.get_corlib || !api.class_from_name || !api.class_get_method_from_name ||
            !api.string_new || !api.class_get_type)
        {
            LOGE("reflection path: required api missing"); return;
        }
        auto corlib = api.get_corlib();
        if (!corlib) { LOGE("get_corlib returned null"); return; }
        auto asm_class    = api.class_from_name(corlib, "System.Reflection", "Assembly");
        if (!asm_class)   { LOGE("Assembly class not found"); return; }
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
            auto ref_asm   = reinterpret_cast<Load_fn>(asm_load->methodPointer)(nullptr, name, nullptr);
            if (!ref_asm) continue;
            auto ref_types = reinterpret_cast<GetTypes_fn>(asm_gettypes->methodPointer)(ref_asm, nullptr);
            if (!ref_types) continue;
            for (size_t j = 0; j < ref_types->max_length; ++j) {
                if (!ref_types->vector[j]) continue;
                auto klass = api.class_from_system_type(
                    static_cast<Il2CppReflectionType*>(ref_types->vector[j]));
                if (!klass) continue;
                auto type = api.class_get_type(klass);
                if (!type) continue;
                auto dumped = dumpType(type);
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
            LOGI("dump.cs written: %zu types -> %s", type_outputs.size(), cs_path.c_str());
        }
    }
    if (cfg.generate_cpp_header)   output::writeCppHeader  (dir, class_registry);
    if (cfg.generate_frida_script) output::writeFridaScript (dir, class_registry);
    LOGI("all outputs written to %s", dir);
}

namespace bridge {

struct NativeBridgeCallbacks {
    uint32_t version;
    void*    initialize;
    void*    (*loadLibrary)(const char*, int);
    void*    (*getTrampoline)(void*, const char*, const char*, uint32_t);
    void*    isSupported; void* getAppEnv; void* isCompatibleWith;
    void*    getSignalHandler; void* unloadLibrary; void* getError;
    void*    isPathSupported; void* initAnonymousNamespace;
    void*    createNamespace; void* linkNamespaces;
    void*    (*loadLibraryExt)(const char*, int, void*);
};

[[nodiscard]] static std::string resolveNativeLibDir(JavaVM* jvm) {
    JNIEnv* env = nullptr;
    jvm->AttachCurrentThread(&env, nullptr);
    if (!env) return {};
    auto atc = env->FindClass("android/app/ActivityThread");
    if (!atc) { LOGE("ActivityThread not found"); return {}; }
    auto cam = env->GetStaticMethodID(atc, "currentApplication", "()Landroid/app/Application;");
    if (!cam) { LOGE("currentApplication not found"); return {}; }
    auto app  = env->CallStaticObjectMethod(atc, cam);
    if (!app) return {};
    auto appc = env->GetObjectClass(app);
    if (!appc) return {};
    auto gaim = env->GetMethodID(appc, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    if (!gaim) return {};
    auto ai   = env->CallObjectMethod(app, gaim);
    if (!ai)  return {};
    auto nldf = env->GetFieldID(env->GetObjectClass(ai), "nativeLibraryDir", "Ljava/lang/String;");
    if (!nldf) return {};
    auto jstr = reinterpret_cast<jstring>(env->GetObjectField(ai, nldf));
    if (!jstr) return {};
    auto cstr = env->GetStringUTFChars(jstr, nullptr);
    if (!cstr) return {};
    LOGI("native lib dir: %s", cstr);
    std::string result(cstr);
    env->ReleaseStringUTFChars(jstr, cstr);
    return result;
}

[[nodiscard]] static std::string readNativeBridgeProp() {
    auto buf = std::array<char, PROP_VALUE_MAX>{};
    __system_property_get("ro.dalvik.vm.native.bridge", buf.data());
    return { buf.data() };
}

static bool loadViaX86(
    const std::string& game_dir, const std::string& out_dir,
    const config::DumperConfig& cfg, int api_level, void* data, size_t length
) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto libart  = dlopen("libart.so", RTLD_NOW);
    if (!libart) return false;
    auto get_vms = reinterpret_cast<jint (*)(JavaVM**, jsize, jsize*)>(
        dlsym(libart, "JNI_GetCreatedJavaVMs"));
    LOGI("JNI_GetCreatedJavaVMs: %p", get_vms);
    if (!get_vms) return false;
    JavaVM* jvm_buf[1]; jsize num = 0;
    if (get_vms(jvm_buf, 1, &num) != JNI_OK || num == 0) { LOGE("GetCreatedJavaVMs failed"); return false; }
    auto jvm     = jvm_buf[0];
    auto lib_dir = resolveNativeLibDir(jvm);
    if (lib_dir.empty()) { LOGE("resolveNativeLibDir failed"); return false; }
    if (lib_dir.find("/lib/x86") != std::string::npos) {
        LOGI("x86 native, NativeBridge not needed"); munmap(data, length); return false;
    }
    void* nb = dlopen("libhoudini.so", RTLD_NOW);
    if (!nb) { auto prop = readNativeBridgeProp(); if (!prop.empty()) nb = dlopen(prop.data(), RTLD_NOW); }
    if (!nb) return false;
    auto cb = reinterpret_cast<NativeBridgeCallbacks*>(dlsym(nb, "NativeBridgeItf"));
    if (!cb) return false;

    // Android 16 uyumlu memfd_create — syscall yerine libc wrapper dene
    int memfd = -1;
#if defined(__ANDROID_API__) && __ANDROID_API__ >= 30
    memfd = memfd_create("anon", MFD_CLOEXEC);
#endif
    if (memfd < 0)
        memfd = static_cast<int>(syscall(__NR_memfd_create, "anon", MFD_CLOEXEC));
    if (memfd < 0) { LOGE("memfd_create failed errno=%d", errno); return false; }

    ftruncate(memfd, static_cast<off_t>(length));
    auto mem = mmap(nullptr, length, PROT_WRITE, MAP_SHARED, memfd, 0);
    if (mem == MAP_FAILED) { close(memfd); return false; }
    memcpy(mem, data, length);
    munmap(mem, length);
    munmap(data, length);
    char fd_path[PATH_MAX];
    snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", memfd);
    void* arm_handle = (api_level >= 26)
        ? cb->loadLibraryExt(fd_path, RTLD_NOW, reinterpret_cast<void*>(3))
        : cb->loadLibrary(fd_path, RTLD_NOW);
    if (!arm_handle) { close(memfd); return false; }
    auto jni_load = reinterpret_cast<void (*)(JavaVM*, void*)>(
        cb->getTrampoline(arm_handle, "JNI_OnLoad", nullptr, 0));
    if (!jni_load) { close(memfd); return false; }
    auto combined = game_dir + "|" + out_dir;
    jni_load(jvm, const_cast<char*>(combined.c_str()));
    close(memfd);
    return true;
}

} // namespace bridge

namespace {

constexpr int     kMaxRetries  = 10;
constexpr int64_t kBaseDelayMs = 500;
constexpr int64_t kMaxDelayMs  = 16000;

[[nodiscard]] int64_t exponentialDelay(int attempt) {
    int64_t d = kBaseDelayMs;
    for (int i = 0; i < attempt; ++i) d *= 2;
    return d < kMaxDelayMs ? d : kMaxDelayMs;
}

static void hackStart(
    const std::string& /*game_dir*/,
    const std::string& out_dir,
    const config::DumperConfig& cfg
) {
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        void* handle = xdl_open("libil2cpp.so", XDL_DEFAULT);
        if (handle) {
            runApiInit(handle);
            xdl_close(handle);
            runDump(out_dir.c_str(), cfg);
            return;
        }
        auto delay = exponentialDelay(attempt);
        LOGI("libil2cpp.so not ready, attempt=%d delay=%" PRId64 "ms", attempt, delay);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    LOGE("libil2cpp.so not found after %d retries tid=%d", kMaxRetries, gettid());
}

static void hackPrepare(
    const std::string& game_dir,
    const std::string& out_dir,
    const config::DumperConfig& cfg,
    void* data, size_t length
) {
    LOGI("hack_prepare tid=%d", gettid());
    int api_level = android_get_device_api_level();
    LOGI("api_level=%d", api_level);
#if defined(__i386__) || defined(__x86_64__)
    if (bridge::loadViaX86(game_dir, out_dir, cfg, api_level, data, length)) return;
#endif
    hackStart(game_dir, out_dir, cfg);
}

} // anonymous namespace

class EaquelDumperModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (!args) return;

        // Denylist kontrolü — process denylist'teyse modülü kapat
        uint32_t flags = api->getFlags();
        if (flags & static_cast<uint32_t>(zygisk::StateFlag::PROCESS_ON_DENYLIST)) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const char* pkg = nullptr;
        const char* dir = nullptr;
        if (args->nice_name)    pkg = env->GetStringUTFChars(args->nice_name,    nullptr);
        if (args->app_data_dir) dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        preSpecialize(pkg ? pkg : "", dir ? dir : "");
        if (pkg) env->ReleaseStringUTFChars(args->nice_name,    pkg);
        if (dir) env->ReleaseStringUTFChars(args->app_data_dir, dir);
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
        if (!enable_hack) return;
        std::thread([
            g  = std::move(game_dir),
            o  = std::move(out_dir),
            c  = active_cfg,
            bd = bridge_data,
            bl = bridge_length
        ]() mutable {
            hackPrepare(g, o, c, bd, bl);
        }).detach();
    }

private:
    zygisk::Api* api           = nullptr;
    JNIEnv*      env           = nullptr;
    bool         enable_hack   = false;
    void*        bridge_data   = nullptr;
    size_t       bridge_length = 0;
    std::string  game_dir;
    std::string  out_dir;
    config::DumperConfig active_cfg;

    void preSpecialize(const char* pkg, const char* app_data_dir) {
        active_cfg = config::loadConfig();
        LOGI("preSpecialize: pkg=%s cfg_target=%s", pkg, active_cfg.target_package.c_str());
        if (!config::isTargetPackage(active_cfg, pkg)) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        LOGI("target detected: %s", pkg);
        enable_hack = true;
        game_dir    = app_data_dir;
        out_dir     = active_cfg.output_dir.empty()
                      ? std::string(config::kFallbackOutput)
                      : active_cfg.output_dir;
        LOGI("output dir: %s", out_dir.c_str());
#if defined(__i386__) || defined(__x86_64__)
        loadArmBridgePayload();
#endif
    }

    void loadArmBridgePayload() {
        const char* path = nullptr;
#if defined(__i386__)
        path = "zygisk/armeabi-v7a.so";
#elif defined(__x86_64__)
        path = "zygisk/arm64-v8a.so";
#else
        return;
#endif
        int dirfd = api->getModuleDir();
        int fd    = openat(dirfd, path, O_RDONLY);
        if (fd == -1) { LOGW("arm bridge not found: %s", path); return; }
        struct stat sb{};
        fstat(fd, &sb);
        bridge_length = static_cast<size_t>(sb.st_size);
        bridge_data   = mmap(nullptr, bridge_length, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (bridge_data == MAP_FAILED) {
            LOGE("mmap arm bridge failed");
            bridge_data   = nullptr;
            bridge_length = 0;
        }
    }
};

REGISTER_ZYGISK_MODULE(EaquelDumperModule)

#if defined(__arm__) || defined(__aarch64__)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    if (!vm) return JNI_ERR;
    std::string combined(reserved ? static_cast<const char*>(reserved) : "");
    auto sep  = combined.find('|');
    auto game = (sep != std::string::npos) ? combined.substr(0, sep) : combined;
    auto out  = (sep != std::string::npos) ? combined.substr(sep + 1) : std::string(config::kFallbackOutput);
    config::DumperConfig cfg;
    cfg.output_dir = out;
    std::thread([g = std::move(game), o = std::move(out), c = std::move(cfg)]() mutable {
        hackStart(g, o, c);
    }).detach();
    return JNI_VERSION_1_6;
}
#endif
