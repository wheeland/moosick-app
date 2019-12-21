TARGET = dbcommon
CONFIG += c++11
TEMPLATE = lib

QT += core

SOURCES += \
    src/library.cpp \

HEADERS += \
    include/type_ids.hpp \
    include/itemcollection.hpp \
    include/library.hpp \

INCLUDEPATH += \
    include/

DESTDIR = ../bin/
