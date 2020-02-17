#!/bin/sh

QT_PATH=/opt/Qt/5.14.1-android

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QT_PATH/lib
CMAKE_PREFIX_PATH=$QT_PATH
QT_PLUGIN_PATH=$QT_PATH/plugins
PATH=$QT_PATH/bin:$PATH

export ANDROID_NDK_ROOT=/home/hagen/software/android/android-ndk-r20
export ANDROID_SDK_ROOT=/home/hagen/software/android/sdk-tools

qmake .. CONFIG+=release -spec android-clang ANDROID_ABIS="armeabi-v7a"
#qmake .. CONFIG+=release CONFIG+=qml_debug -spec android-clang ANDROID_ABIS="armeabi-v7a"

make -j9 qmake_all
make -j9
make install INSTALL_ROOT=android-build

androiddeployqt \
    --input app/android-moosick-deployment-settings.json \
    --output android-build \
    --android-platform android-23 \
    --jdk /usr/lib/jvm/java-8-openjdk-amd64 \
    --gradle \
    --aab \
    --jarsigner

cp ./android-build/build/outputs/apk/debug/android-build-debug.apk moosickapp.apk

$ANDROID_SDK_ROOT/platform-tools/adb uninstall de.wheeland.moosick
$ANDROID_SDK_ROOT/platform-tools/adb install moosickapp.apk
