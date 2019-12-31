TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = \
    dbcommon \
    netcommon \
    clientcommon \
    dbserver \
    testclient \
    app \
    cgi \

contains(ANDROID_TARGET_ARCH, armeabi-v7a) {
    DISTFILES += \
        android/AndroidManifest.xml \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradlew \
        android/res/values/libs.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew.bat

    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}
