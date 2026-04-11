plugins {
    id("com.android.library") version "9.1.0" apply false
}

val outDir by extra { file("$rootDir/out") }

tasks.register<Delete>("clean") {
    delete(layout.buildDirectory, outDir)
}
