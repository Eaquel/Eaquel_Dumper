plugins {
    id("com.android.library") version libs.versions.agp.get() apply false
}

apply(from = "module.gradle.kts")

val outDir by extra { file("$rootDir/out") }

tasks.register<Delete>("clean") {
    delete(layout.buildDirectory, outDir)
    doLast { logger.lifecycle("🧹 Clean complete: build/ + out/ removed.") }
}

gradle.projectsEvaluated {
    logger.lifecycle("─────────────────────────────────────────────────────")
    logger.lifecycle("  🦊 Eaquel Dumper — Root project configured")
    logger.lifecycle("  AGP   : ${libs.versions.agp.get()}")
    logger.lifecycle("  Gradle: ${gradle.gradleVersion}")
    logger.lifecycle("─────────────────────────────────────────────────────")
}
