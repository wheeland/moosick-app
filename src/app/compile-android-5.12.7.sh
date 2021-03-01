#!/bin/sh

QT_PATH=/opt/Qt/5.12.10-android

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$QT_PATH/lib
CMAKE_PREFIX_PATH=$QT_PATH
QT_PLUGIN_PATH=$QT_PATH/plugins
PATH=$QT_PATH/bin:$PATH

export ANDROID_NDK_ROOT=/home/hagen/software/android-ndk-r21e/
export ANDROID_SDK_ROOT=/home/hagen/software/android-sdk/

#qmake .. CONFIG+=release
qmake .. CONFIG+=release -spec android-clang ANDROID_ABIS="armeabi-v7a"

make -j9 qmake_all
make -j9
make install INSTALL_ROOT=`pwd`

androiddeployqt \
    --input app/android-libmoosick.so-deployment-settings.json \
    --output `pwd` \
    --gradle \
    --android-platform android-23 \

cp build/outputs/apk/*/*.apk moosickapp.apk

$ANDROID_SDK_ROOT/platform-tools/adb uninstall de.wheeland.moosick
$ANDROID_SDK_ROOT/platform-tools/adb install moosickapp.apk
