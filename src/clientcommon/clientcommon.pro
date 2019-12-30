TARGET = clientcommon
CONFIG += c++11 staticlib
TEMPLATE = lib

QT += core network

SOURCES += \
    src/messages.cpp \
    src/download.cpp \
    src/util.cpp \

HEADERS += \
    include/messages.hpp \
    include/download.hpp \
    src/util.hpp \

INCLUDEPATH += \
    include/ \
    ../dbcommon/include/ \
    ../netcommon/ \

LIBS += \
    -L../bin/ -ldbcommon -lnetcommon

DESTDIR = ../bin/
