plugins {
    id("com.android.application") version "8.9.1" apply false
}
tasks.register<Delete>("clean") {
    delete(rootProject.buildDir)
}
