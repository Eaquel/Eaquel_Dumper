import java.nio.file.Files
import java.nio.file.Paths
import java.nio.file.StandardCopyOption
import org.apache.tools.ant.filters.FixCrLfFilter

plugins {
    alias(libs.plugins.android.library)
}

val moduleLibraryName: String by rootProject.extra
val magiskModuleId:    String by rootProject.extra
val moduleName:        String by rootProject.extra
val moduleAuthor:      String by rootProject.extra
val moduleDescription: String by rootProject.extra
val moduleVersion:     String by rootProject.extra
val moduleVersionCode: String by rootProject.extra
val outDir:            File   by rootProject.extra

android {
    namespace = "com.eaquel.dumper"
    compileSdk = libs.versions.compileSdk.get().toInt()
    ndkVersion = libs.versions.ndkVersion.get()

    compileOptions {
        sourceCompatibility = JavaVersion.toVersion(25)
        targetCompatibility = JavaVersion.toVersion(25)
    }

    kotlin { jvmToolchain(25) }

    defaultConfig {
        minSdk = libs.versions.minSdk.get().toInt()

        externalNativeBuild {
            cmake {
                arguments(
                    "-DMODULE_NAME:STRING=$moduleLibraryName",
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DCMAKE_MINIMUM_REQUIRED_VERSION=${libs.versions.cmakeVersion.get()}",
                    "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON",
                    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
                )
                abiFilters("arm64-v8a", "armeabi-v7a", "x86_64")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            externalNativeBuild {
                cmake {
                    arguments("-DCMAKE_BUILD_TYPE=Release")
                }
            }
        }
    }

    sourceSets {
        getByName("main") {
            manifest.srcFile("Source/Main/AndroidManifest.xml")
        }
    }

    buildFeatures {
        prefab = false
        buildConfig = false
    }

    externalNativeBuild {
        cmake {
            path = file("Source/Main/Native/CMakeLists.txt")
            version = libs.versions.cmakeVersion.get()
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
    }

    lint {
        abortOnError = false
    }
}

androidComponents {
    onVariants { variant ->
        val variantCapped  = variant.name.replaceFirstChar { it.uppercase() }
        val variantLower   = variant.name.lowercase()
        val skeletonDir       = outDir
        val skeletonZygiskDir = skeletonDir.resolve("zygisk")
        val capturedLibName   = moduleLibraryName

        val prepareTask = tasks.register<Sync>("prepareSkeleton$variantCapped") {
            dependsOn("strip${variantCapped}DebugSymbols")
            into(skeletonDir)

            from("$rootDir/Base") {
                exclude("module.prop")
            }

            from("$rootDir/Base") {
                include("module.prop")
                expand(
                    mapOf(
                        "id"          to magiskModuleId,
                        "name"        to moduleName,
                        "version"     to moduleVersion,
                        "versionCode" to moduleVersionCode,
                        "author"      to moduleAuthor,
                        "description" to moduleDescription
                    )
                )
                filter(
                    mapOf("eol" to FixCrLfFilter.CrLf.newInstance("lf")),
                    FixCrLfFilter::class.java
                )
            }

            from(layout.buildDirectory.dir(
                "intermediates/stripped_native_libs/$variantLower/strip${variantCapped}DebugSymbols/out/lib"
            )) {
                into("lib")
            }

            doLast {
                skeletonZygiskDir.mkdirs()
                val skeletonLibDir = skeletonDir.resolve("lib")

                skeletonLibDir.listFiles()
                    ?.filter { it.isDirectory }
                    ?.forEach { abiDir ->
                        val src = Paths.get("${abiDir.absolutePath}/lib${capturedLibName}.so")
                        val dst = Paths.get("${skeletonZygiskDir.absolutePath}/${abiDir.name}.so")

                        Files.createDirectories(dst.parent)
                        if (src.toFile().exists()) {
                            Files.copy(src, dst, StandardCopyOption.REPLACE_EXISTING)
                            Files.delete(src)
                        }
                    }
                skeletonLibDir.deleteRecursively()
            }
        }

        tasks.matching { it.name == "assemble$variantCapped" }.configureEach {
            finalizedBy(prepareTask)
        }
    }
}
