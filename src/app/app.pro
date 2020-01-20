TARGET = moosick
CONFIG += c++11
TEMPLATE = app

QT += core quick qml network multimedia virtualkeyboard
QTPLUGIN += qtvirtualkeyboardplugin

# force application integration for QtVirtualKeyboard
CONFIG += disable-desktop

SOURCES += \
    src/main.cpp \
    src/httpclient.cpp \
    src/search.cpp \
    src/playlist.cpp \
    src/controller.cpp \
    src/audio.cpp \
    src/database.cpp \
    src/stringmodel.cpp \
    src/util/androidutil.cpp \
    src/util/qmlutil.cpp \
    src/util/modeladapter.cpp \

HEADERS += \
    src/search.hpp \
    src/httpclient.hpp \
    src/playlist.hpp \
    src/controller.hpp \
    src/audio.hpp \
    src/database.hpp \
    src/database_items.hpp \
    src/stringmodel.hpp \
    src/util/androidutil.hpp \
    src/util/qmlutil.hpp \
    src/util/modeladapter.hpp \

RESOURCES += \
    qml.qrc \
    data.qrc \

INCLUDEPATH += \
    ../dbcommon/include/ \
    ../netcommon/ \

LIBS += \
    -L../bin/ -ldbcommon -lnetcommon

DESTDIR = ../bin/
