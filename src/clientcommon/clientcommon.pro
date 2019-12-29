TARGET = clientcommon
CONFIG += c++11
TEMPLATE = lib

QT += core network

SOURCES += \
    messages.cpp \
    download.cpp \

HEADERS += \
    messages.hpp \
    download.hpp \

INCLUDEPATH += \
    ../dbcommon/include/

LIBS += \
    -L../bin/ -ldbcommon

DESTDIR = ../bin/
