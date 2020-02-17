TARGET = moosick
CONFIG += c++11
TEMPLATE = app

QT += core quick qml network multimedia

!android {
    QT += virtualkeyboard
    QTPLUGIN += qtvirtualkeyboardplugin

    # force application integration for QtVirtualKeyboard
    CONFIG += disable-desktop
}

SOURCES += \
    src/audio.cpp \
    src/controller.cpp \
    src/httpclient.cpp \
    src/main.cpp \
    src/multichoicecontroller.cpp \
    src/playlist.cpp \
    src/search.cpp \
    src/selecttagsmodel.cpp \
    src/stringmodel.cpp \
    src/database/database.cpp \
    src/database/database_items.cpp \
    src/database/database_interface.cpp \
    src/util/androidutil.cpp \
    src/util/qmlutil.cpp \
    src/util/modeladapter.cpp \

HEADERS += \
    src/audio.hpp \
    src/controller.hpp \
    src/httpclient.hpp \
    src/multichoicecontroller.hpp \
    src/playlist.hpp \
    src/search.hpp \
    src/selecttagsmodel.hpp \
    src/stringmodel.hpp \
    src/database/database.hpp \
    src/database/database_items.hpp \
    src/database/database_interface.hpp \
    src/util/androidutil.hpp \
    src/util/qmlutil.hpp \
    src/util/modeladapter.hpp \

RESOURCES += \
    qml.qrc \
    data.qrc \

INCLUDEPATH += \
    ../common/include/

LIBS += \
    -L../bin/ -lcommon

DESTDIR = ../bin/

android {
    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew \
        android/gradlew.bat \
        android/res/values/libs.xml

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    ANDROID_EXTRA_LIBS = bin/libcommon.so
}
