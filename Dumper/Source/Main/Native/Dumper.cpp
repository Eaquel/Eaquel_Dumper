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

}

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

static std::vector<MethodEntry> collectMethods(Il2CppClass* klass) {
    auto& api = il2cpp_api::g_api;
    std::vector<MethodEntry> entries;
    void* iter = nullptr;
    while (auto method = api.class_get_methods(klass, &iter)) {
        MethodEntry e;
        e.va  = method->methodPointer ? reinterpret_cast<uint64_t>(method->methodPointer) : 0;
        e.rva = (e.va && il2cpp_api::g_il2cpp_base) ? (e.va - il2cpp_api::g_il2cpp_base) : 0;
        uint32_t iflags = 0;
        e.modifier    = buildMethodModifier(api.method_get_flags(method, &iflags));
        e.name        = api.method_get_name(method) ? api.method_get_name(method) : "";
        auto ret      = api.method_get_return_type(method);
        e.return_type = api.class_get_name(api.class_from_type(ret));
        auto pc = api.method_get_param_count(method);
        for (uint32_t i = 0; i < pc; ++i) {
            auto param = api.method_get_param(method, i);
            auto pname = api.method_get_param_name(method, i);
            auto tname = api.class_get_name(api.class_from_type(param));
            e.params.emplace_back(
                tname ? tname : "object",
                pname ? pname : ("p" + std::to_string(i))
            );
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
    auto& api = il2cpp_api::g_api;
    std::stringstream out;
    out << "\n\t// Properties\n";
    void* iter = nullptr;
    while (auto pc = api.class_get_properties(klass, &iter)) {
        auto prop  = const_cast<PropertyInfo*>(pc);
        auto get_m = api.property_get_get_method(prop);
        auto set_m = api.property_get_set_method(prop);
        auto pname = api.property_get_name(prop);
        out << "\t";
        Il2CppClass* prop_class = nullptr;
        uint32_t iflags = 0;
        if (get_m) {
            out << buildMethodModifier(api.method_get_flags(get_m, &iflags));
            prop_class = api.class_from_type(api.method_get_return_type(get_m));
        } else if (set_m) {
            out << buildMethodModifier(api.method_get_flags(set_m, &iflags));
            prop_class = api.class_from_type(api.method_get_param(set_m, 0));
        }
        if (prop_class) {
            out << api.class_get_name(prop_class) << " " << pname << " { ";
            if (get_m) out << "get; ";
            if (set_m) out << "set; ";
            out << "}\n";
        } else if (pname) {
            out << " // unknown property " << pname << "\n";
        }
    }
    return out.str();
}

static std::string dumpFields(Il2CppClass* klass) {
    auto& api    = il2cpp_api::g_api;
    auto is_enum = api.class_is_enum(klass);
    std::stringstream out;
    out << "\n\t// Fields\n";
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
        auto fc = api.class_from_type(api.field_get_type(field));
        out << api.class_get_name(fc) << " " << api.field_get_name(field);
        if ((attrs & FIELD_ATTRIBUTE_LITERAL) && is_enum) {
            uint64_t val = 0;
            api.field_static_get_value(field, &val);
            out << " = " << std::dec << val;
        }
        out << "; // 0x" << std::hex << api.field_get_offset(field) << "\n";
    }
    return out.str();
}

static std::string dumpType(const Il2CppType* type) {
    auto& api        = il2cpp_api::g_api;
    auto  klass      = api.class_from_type(type);
    auto  flags      = api.class_get_flags(klass);
    auto  is_vt      = api.class_is_valuetype(klass);
    auto  is_enum    = api.class_is_enum(klass);
    std::stringstream out;
    out << "\n// Namespace: " << api.class_get_namespace(klass) << "\n";
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
    } else if (!(flags & TYPE_ATTRIBUTE_INTERFACE) && (flags & TYPE_ATTRIBUTE_ABSTRACT)) {
        out << "abstract ";
    } else if (!is_vt && !is_enum && (flags & TYPE_ATTRIBUTE_SEALED)) {
        out << "sealed ";
    }
    if (flags & TYPE_ATTRIBUTE_INTERFACE) out << "interface ";
    else if (is_enum)                     out << "enum ";
    else if (is_vt)                       out << "struct ";
    else                                  out << "class ";
    out << api.class_get_name(klass);
    std::vector<std::string> extends;
    auto parent = api.class_get_parent(klass);
    if (!is_vt && !is_enum && parent) {
        auto pt = api.class_get_type(parent);
        if (pt->type != IL2CPP_TYPE_OBJECT)
            extends.emplace_back(api.class_get_name(parent));
    }
    void* iter = nullptr;
    while (auto itf = api.class_get_interfaces(klass, &iter))
        extends.emplace_back(api.class_get_name(itf));
    if (!extends.empty()) {
        out << " : " << extends[0];
        for (size_t i = 1; i < extends.size(); ++i) out << ", " << extends[i];
    }
    out << "\n{";
    out << dumpFields(klass);
    out << dumpProperties(klass);
    out << dumpMethods(klass);
    out << "}\n";
    return out.str();
}

namespace output {

static void ensureDirectory(const std::string& path) { mkdir(path.c_str(), 0755); }

static void writeCppHeader(
    const std::string& dir,
    const std::vector<std::pair<Il2CppClass*, std::string>>& classes
) {
    auto path = dir + "/dump.h";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("failed to open %s", path.c_str()); return; }
    f << "#pragma once\n#include <cstdint>\n\n";
    auto& api = il2cpp_api::g_api;
    for (const auto& [klass, dll] : classes) {
        if (!klass) continue;
        auto cn = api.class_get_name(klass);
        auto ns = api.class_get_namespace(klass);
        if (!cn) continue;
        f << "// DLL: " << dll << "\n";
        if (ns && ns[0] != '\0') f << "// Namespace: " << ns << "\n";
        f << "struct " << cn << " {\n";
        void* iter = nullptr;
        while (auto field = api.class_get_fields(klass, &iter)) {
            auto fn  = api.field_get_name(field);
            auto fo  = api.field_get_offset(field);
            auto fc  = api.class_from_type(api.field_get_type(field));
            auto tn  = fc ? api.class_get_name(fc) : "void*";
            if (!fn) continue;
            f << "    // 0x" << std::hex << fo << "\n";
            f << "    // " << (tn ? tn : "unknown") << " " << fn << ";\n";
        }
        for (const auto& m : collectMethods(klass)) {
            if (m.rva == 0) continue;
            f << "    // " << m.modifier << m.return_type << " " << m.name << "(";
            for (size_t i = 0; i < m.params.size(); ++i) {
                if (i > 0) f << ", ";
                f << m.params[i].first << " " << m.params[i].second;
            }
            f << "); // RVA: 0x" << std::hex << m.rva << "\n";
        }
        f << "};\n\n";
    }
    f.close();
    LOGI("dump.h written to %s", path.c_str());
}

static void writeFridaScript(
    const std::string& dir,
    const std::vector<std::pair<Il2CppClass*, std::string>>& classes
) {
    auto path = dir + "/dump.js";
    std::ofstream f(path);
    if (!f.is_open()) { LOGE("failed to open %s", path.c_str()); return; }
    f << "\"use strict\";\n\nconst il2cppBase = Module.findBaseAddress(\"libil2cpp.so\");\n\n";
    auto& api = il2cpp_api::g_api;
    for (const auto& [klass, dll] : classes) {
        if (!klass) continue;
        auto cn = api.class_get_name(klass);
        if (!cn) continue;
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

}

static void runApiInit(void* handle) {
    Dl_info di{};
    if (dladdr(handle, &di))
        il2cpp_api::g_il2cpp_base = reinterpret_cast<uint64_t>(di.dli_fbase);
    LOGI("il2cpp handle: %p  base: 0x%" PRIx64, handle, il2cpp_api::g_il2cpp_base);
    il2cpp_api::populateFunctionTable(handle);
    auto& api = il2cpp_api::g_api;
    if (!api.domain_get_assemblies) { LOGE("il2cpp api init failed"); return; }
    while (!api.is_vm_thread(nullptr)) { LOGI("waiting for il2cpp..."); sleep(1); }
    api.thread_attach(api.domain_get());
}

static void runDump(const char* out_dir, const config::DumperConfig& cfg) {
    auto& api = il2cpp_api::g_api;
    LOGI("dump started -> %s", out_dir);
    output::ensureDirectory(out_dir);
    size_t ac = 0;
    auto domain     = api.domain_get();
    auto assemblies = api.domain_get_assemblies(domain, &ac);
    std::stringstream img_header;
    for (size_t i = 0; i < ac; ++i) {
        auto img = api.assembly_get_image(assemblies[i]);
        img_header << "// Image " << i << ": " << api.image_get_name(img) << "\n";
    }
    std::vector<std::string>                          type_outputs;
    std::vector<std::pair<Il2CppClass*, std::string>> class_registry;

    if (api.image_get_class) {
        LOGI("il2cpp >= 2018.3 path");
        for (size_t i = 0; i < ac; ++i) {
            auto img    = api.assembly_get_image(assemblies[i]);
            auto dll    = std::string(api.image_get_name(img));
            auto hdr    = "\n// Dll : " + dll;
            auto cc     = api.image_get_class_count(img);
            for (size_t j = 0; j < cc; ++j) {
                auto klass = const_cast<Il2CppClass*>(api.image_get_class(img, j));
                type_outputs.push_back(hdr + dumpType(api.class_get_type(klass)));
                class_registry.emplace_back(klass, dll);
            }
        }
    } else {
        LOGI("il2cpp < 2018.3 reflection path");
        auto corlib      = api.get_corlib();
        auto asm_class   = api.class_from_name(corlib, "System.Reflection", "Assembly");
        auto asm_load    = api.class_get_method_from_name(asm_class, "Load", 1);
        auto asm_gettypes = api.class_get_method_from_name(asm_class, "GetTypes", 0);
        if (!asm_load || !asm_load->methodPointer)     { LOGE("Assembly::Load not found");     return; }
        if (!asm_gettypes || !asm_gettypes->methodPointer) { LOGE("Assembly::GetTypes not found"); return; }
        using Load_fn     = void* (*)(void*, Il2CppString*, void*);
        using GetTypes_fn = Il2CppArray* (*)(void*, void*);
        for (size_t i = 0; i < ac; ++i) {
            auto img  = api.assembly_get_image(assemblies[i]);
            auto dll  = std::string(api.image_get_name(img));
            auto hdr  = "\n// Dll : " + dll;
            auto dot  = dll.rfind('.');
            auto name = api.string_new(dll.substr(0, dot).data());
            auto ref_asm   = reinterpret_cast<Load_fn>(asm_load->methodPointer)(nullptr, name, nullptr);
            auto ref_types = reinterpret_cast<GetTypes_fn>(asm_gettypes->methodPointer)(ref_asm, nullptr);
            for (size_t j = 0; j < ref_types->max_length; ++j) {
                auto klass = api.class_from_system_type(
                    static_cast<Il2CppReflectionType*>(ref_types->vector[j]));
                type_outputs.push_back(hdr + dumpType(api.class_get_type(klass)));
                class_registry.emplace_back(klass, dll);
            }
        }
    }

    {
        auto cs_path = std::string(out_dir) + "/dump.cs";
        std::ofstream cs(cs_path);
        cs << img_header.str();
        for (const auto& e : type_outputs) cs << e;
        cs.close();
        LOGI("dump.cs written to %s", cs_path.c_str());
    }
    if (cfg.generate_cpp_header)   output::writeCppHeader(out_dir, class_registry);
    if (cfg.generate_frida_script) output::writeFridaScript(out_dir, class_registry);
    LOGI("all outputs written to %s", out_dir);
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
    auto atc = env->FindClass("android/app/ActivityThread");
    if (!atc) { LOGE("ActivityThread not found"); return {}; }
    auto cam = env->GetStaticMethodID(atc, "currentApplication", "()Landroid/app/Application;");
    if (!cam) { LOGE("currentApplication not found"); return {}; }
    auto app  = env->CallStaticObjectMethod(atc, cam);
    auto appc = env->GetObjectClass(app);
    if (!appc) { LOGE("application class not found"); return {}; }
    auto gaim = env->GetMethodID(appc, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    if (!gaim) { LOGE("getApplicationInfo not found"); return {}; }
    auto ai   = env->CallObjectMethod(app, gaim);
    auto nldf = env->GetFieldID(env->GetObjectClass(ai), "nativeLibraryDir", "Ljava/lang/String;");
    if (!nldf) { LOGE("nativeLibraryDir not found"); return {}; }
    auto jstr = reinterpret_cast<jstring>(env->GetObjectField(ai, nldf));
    auto cstr = env->GetStringUTFChars(jstr, nullptr);
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
    auto libart = dlopen("libart.so", RTLD_NOW);
    auto get_vms = reinterpret_cast<jint (*)(JavaVM**, jsize, jsize*)>(
        dlsym(libart, "JNI_GetCreatedJavaVMs"));
    LOGI("JNI_GetCreatedJavaVMs: %p", get_vms);
    JavaVM* jvm_buf[1]; jsize num = 0;
    if (get_vms(jvm_buf, 1, &num) != JNI_OK || num == 0) { LOGE("GetCreatedJavaVMs failed"); return false; }
    auto jvm     = jvm_buf[0];
    auto lib_dir = resolveNativeLibDir(jvm);
    if (lib_dir.empty()) { LOGE("resolveNativeLibDir failed"); return false; }
    if (lib_dir.find("/lib/x86") != std::string::npos) {
        LOGI("x86 native, NativeBridge not needed"); munmap(data, length); return false;
    }
    void* nb = dlopen("libhoudini.so", RTLD_NOW);
    if (!nb) { auto prop = readNativeBridgeProp(); nb = dlopen(prop.data(), RTLD_NOW); }
    if (!nb) return false;
    auto cb = reinterpret_cast<NativeBridgeCallbacks*>(dlsym(nb, "NativeBridgeItf"));
    if (!cb) return false;
    int memfd = static_cast<int>(syscall(__NR_memfd_create, "anon", MFD_CLOEXEC));
    ftruncate(memfd, static_cast<off_t>(length));
    auto mem = mmap(nullptr, length, PROT_WRITE, MAP_SHARED, memfd, 0);
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
    auto combined = game_dir + "|" + out_dir;
    jni_load(jvm, const_cast<char*>(combined.c_str()));
    return true;
}

}

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
    const std::string& game_dir,
    const std::string& out_dir,
    const config::DumperConfig& cfg
) {
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        void* handle = xdl_open("libil2cpp.so", XDL_DEFAULT);
        if (handle) {
            runApiInit(handle);
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

}

class EaquelDumperModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        auto pkg = env->GetStringUTFChars(args->nice_name,  nullptr);
        auto dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        preSpecialize(pkg, dir);
        env->ReleaseStringUTFChars(args->nice_name,    pkg);
        env->ReleaseStringUTFChars(args->app_data_dir, dir);
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
        if (!config::isTargetPackage(active_cfg, pkg)) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }
        LOGI("target detected: %s", pkg);
        enable_hack = true;
        game_dir    = app_data_dir;
        out_dir     = active_cfg.output_dir;
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
    }
};

REGISTER_ZYGISK_MODULE(EaquelDumperModule)

#if defined(__arm__) || defined(__aarch64__)
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    std::string combined(reserved ? static_cast<const char*>(reserved) : "");
    auto sep  = combined.find('|');
    auto game = (sep != std::string::npos) ? combined.substr(0, sep) : combined;
    auto out  = (sep != std::string::npos) ? combined.substr(sep + 1) : combined;
    config::DumperConfig cfg;
    cfg.output_dir = out;
    std::thread([g = std::move(game), o = std::move(out), c = std::move(cfg)]() mutable {
        hackStart(g, o, c);
    }).detach();
    return JNI_VERSION_1_6;
}
#endif
