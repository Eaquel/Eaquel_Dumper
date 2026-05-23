buildscript {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
    dependencies {
        classpath(libs.kotlin.gradle.plugin)
    }
}

apply(from = "module.gradle.kts")

val outDir by extra { layout.projectDirectory.dir("out").asFile }

tasks.register<Delete>("clean") {
    description = "Cleans The Build And Output Directories."
    group = "build"
    delete(layout.buildDirectory)
    delete(outDir)
}
