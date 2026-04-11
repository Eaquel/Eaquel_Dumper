import org.apache.tools.ant.filters.FixCrLfFilter
import java.nio.file.Files
import java.nio.file.Paths

plugins {
    id("com.android.library")
}

val moduleLibraryName: String by rootProject.extra
val magiskModuleId: String by rootProject.extra
val moduleName: String by rootProject.extra
val moduleAuthor: String by rootProject.extra
val moduleDescription: String by rootProject.extra
val moduleVersion: String by rootProject.extra
val moduleVersionCode: String by rootProject.extra

val outDir: File by rootProject.extra

android {
    namespace = "com.eaquel.dumper"
    compileSdk = 36
    ndkVersion = "29.0.14206865"

    defaultConfig {
        minSdk = 30

        externalNativeBuild {
            cmake {
                arguments(
                    "-DMODULE_NAME:STRING=$moduleLibraryName",
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON",
                    "-DCMAKE_BUILD_TYPE=Release"
                )
                cppFlags("-std=c++20", "-O3", "-fvisibility=hidden", "-flto")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    buildFeatures {
        prefab = true
    }

    externalNativeBuild {
        cmake {
            path = file("Source/Main/Native/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = false
            pickFirsts += "**/libxdl.so"
        }
    }
}

dependencies {
    implementation("io.github.hexhacking:xdl:2.3.0")
}

androidComponents {
    onVariants { variant ->
        val variantCapped = variant.name.replaceFirstChar { it.uppercase() }
        val variantLower = variant.name.lowercase()
        val zipFileName = "${magiskModuleId.replace('_', '-')}-${moduleVersion}-${variantLower}.zip"
        val skeletonDir = file("$outDir/skeleton_$variantLower")

        val skeletonLibDir = skeletonDir.resolve("lib")
        val skeletonZygiskDir = skeletonDir.resolve("zygisk")
        val capturedLibraryName = moduleLibraryName

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
                        "id" to magiskModuleId,
                        "name" to moduleName,
                        "version" to moduleVersion,
                        "versionCode" to moduleVersionCode,
                        "author" to moduleAuthor,
                        "description" to moduleDescription
                    )
                )
                filter(mapOf("eol" to FixCrLfFilter.CrLf.newInstance("lf")), FixCrLfFilter::class.java)
            }
            from(layout.buildDirectory.dir("intermediates/stripped_native_libs/$variantLower/strip${variantCapped}DebugSymbols/out/lib")) {
                into("lib")
            }

            doLast {
                skeletonZygiskDir.mkdirs()
                skeletonLibDir.listFiles()
                    ?.filter { it.isDirectory }
                    ?.forEach { abiDir ->
                        val src = Paths.get(abiDir.absolutePath + "/lib" + capturedLibraryName + ".so")
                        val dst = Paths.get(skeletonZygiskDir.absolutePath + "/" + abiDir.name + ".so")
                        Files.createDirectories(dst.parent)
                        if (src.toFile().exists()) Files.move(src, dst)
                    }
                skeletonLibDir.deleteRecursively()
            }
        }

        val zipTask = tasks.register<Zip>("zip$variantCapped") {
            dependsOn(prepareTask)
            from(skeletonDir)
            archiveFileName.set(zipFileName)
            destinationDirectory.set(outDir)
        }

        tasks.matching { it.name == "assemble$variantCapped" }.configureEach {
            finalizedBy(zipTask)
        }

        tasks.register<Exec>("push$variantCapped") {
            dependsOn(zipTask)
            workingDir(outDir)
            commandLine("adb", "push", zipFileName, "/data/local/tmp/")
        }

        tasks.register<Exec>("flash$variantCapped") {
            dependsOn("push$variantCapped")
            commandLine(
                "adb", "shell", "su", "-c",
                "magisk --install-module /data/local/tmp/$zipFileName"
            )
        }

        tasks.register<Exec>("flashAndReboot$variantCapped") {
            dependsOn("flash$variantCapped")
            commandLine("adb", "shell", "reboot")
        }
    }
}
