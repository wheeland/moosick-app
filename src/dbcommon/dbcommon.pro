TARGET = dbcommon
CONFIG += c++11 staticlib
TEMPLATE = lib

QT += core

SOURCES += \
    src/library.cpp \
    src/library_serialize.cpp \
    src/jsonconv.cpp \

HEADERS += \
    include/type_ids.hpp \
    include/itemcollection.hpp \
    include/library.hpp \
    include/jsonconv.hpp \
    src/library_priv.hpp \

INCLUDEPATH += \
    include/

DESTDIR = ../bin/
