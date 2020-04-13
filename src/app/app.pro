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
    src/storage.cpp \
    src/stringmodel.cpp \
    src/database/database.cpp \
    src/database/database_items.cpp \
    src/database/database_interface.cpp \
    src/util/androidutil.cpp \
    src/util/qmlutil.cpp \
    src/util/modeladapter.cpp \
    \
    src/gumbo/attribute.c \
    src/gumbo/char_ref.c \
    src/gumbo/error.c \
    src/gumbo/parser.c \
    src/gumbo/string_buffer.c \
    src/gumbo/string_piece.c \
    src/gumbo/tag.c \
    src/gumbo/tokenizer.c \
    src/gumbo/utf8.c \
    src/gumbo/util.c \
    src/gumbo/vector.c \
    \
    src/musicscrape/musicscrape.cpp \
    src/musicscrape/qmusicscrape.cpp \

HEADERS += \
    src/audio.hpp \
    src/controller.hpp \
    src/httpclient.hpp \
    src/multichoicecontroller.hpp \
    src/playlist.hpp \
    src/search.hpp \
    src/selecttagsmodel.hpp \
    src/storage.hpp \
    src/stringmodel.hpp \
    src/database/database.hpp \
    src/database/database_items.hpp \
    src/database/database_interface.hpp \
    src/util/androidutil.hpp \
    src/util/qmlutil.hpp \
    src/util/modeladapter.hpp \
    \
    src/musicscrape/musicscrape.hpp \
    src/musicscrape/qmusicscrape.hpp \

RESOURCES += \
    qml.qrc \
    data.qrc \

INCLUDEPATH += \
    ../common/include/ \
    src/gumbo/ \
    src/ \

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
