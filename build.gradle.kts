plugins {
    id("com.android.library") version "9.1.0" apply false
}

apply(from = "module.gradle.kts")

val outDir by extra { file("$rootDir/out") }

tasks.register<Delete>("clean") {
    delete(layout.buildDirectory, outDir)
}