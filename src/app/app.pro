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
    ../shared/jsonconv.cpp \
    ../shared/library.cpp \
    ../shared/library_serialize.cpp \
    ../shared/serversettings.cpp \
    ../shared/tcpclientserver.cpp \
    \
    ../../3rdparty/gumbo-parser/src/attribute.c \
    ../../3rdparty/gumbo-parser/src/char_ref.c \
    ../../3rdparty/gumbo-parser/src/error.c \
    ../../3rdparty/gumbo-parser/src/parser.c \
    ../../3rdparty/gumbo-parser/src/string_buffer.c \
    ../../3rdparty/gumbo-parser/src/string_piece.c \
    ../../3rdparty/gumbo-parser/src/tag.c \
    ../../3rdparty/gumbo-parser/src/tokenizer.c \
    ../../3rdparty/gumbo-parser/src/utf8.c \
    ../../3rdparty/gumbo-parser/src/util.c \
    ../../3rdparty/gumbo-parser/src/vector.c \
    \
    ../../3rdparty/cpp-musicscrape/musicscrape/musicscrape.cpp \

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
    ../shared/flatmap.hpp \
    ../shared/jsonconv.hpp \
    ../shared/library.hpp \
    ../shared/library_types.hpp \
    ../shared/library_messages.hpp \
    ../shared/serversettings.hpp \
    ../shared/tcpclientserver.hpp \
    ../shared/result.hpp \

RESOURCES += \
    qml.qrc \
    data.qrc \

INCLUDEPATH += \
    src/ \
    ../shared/ \
    ../../3rdparty/gumbo-parser/src/ \
    ../../3rdparty/rapidjson/include/ \
    ../../3rdparty/cpp-musicscrape/ \

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

    include(../../3rdparty/android_openssl/openssl.pri)

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}
