val moduleLibraryName  by extra("eaqueldumper")
val magiskModuleId     by extra("eaquel_dumper")
val moduleName         by extra("Eaquel_Dumper")
val moduleAuthor       by extra("Eaquel")
val moduleVersion      by extra("v1.0.0")
val moduleVersionCode  by extra("1")
val _descOverride: String? = findProperty("moduleDescriptionOverride") as String?
val moduleDescription by extra(
    _descOverride?.trim()?.takeIf { it.isNotBlank() }
        ?: "Runtime IL2CPP binary resolver — JSON-config driven — API 30-36."
)
