TARGET = netcommon
CONFIG += c++11
TEMPLATE = lib

QT += core network

SOURCES += \
    messages.cpp \

HEADERS += \
    messages.hpp \

INCLUDEPATH += \
    ../dbcommon/include/

LIBS += \
    -L../bin/ -ldbcommon

DESTDIR = ../bin/
