plugins {
    alias(libs.plugins.android.library) apply false
}

apply(from = "module.gradle.kts")

val outDir by extra { layout.projectDirectory.dir("out") }

tasks.register<Delete>("clean") {
    description = "Cleans The Build And Output Directories."
    group = "build"
    delete(layout.buildDirectory)
    delete(outDir)
}
