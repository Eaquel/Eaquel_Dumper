plugins {
    alias(libs.plugins.android.library) apply false
}

val outDir by extra { file("$rootDir/out") }

tasks.register<Delete>("clean") {
    delete(layout.buildDirectory, outDir)
}
