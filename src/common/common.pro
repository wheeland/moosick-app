TARGET = common
CONFIG += c++11
TEMPLATE = lib

QT -= gui
QT += core network

SOURCES += \
    src/download.cpp \
    src/jsonconv.cpp \
    src/json.cpp \
    src/library.cpp \
    src/library_serialize.cpp \
    src/messages.cpp \
    src/tcpserver.cpp \
    src/requests.cpp \
    src/util.cpp \

HEADERS += \
    include/download.hpp \
    include/itemcollection.hpp \
    include/jsonconv.hpp \
    include/json.hpp \
    include/library.hpp \
    include/messages.hpp \
    include/tcpserver.hpp \
    include/requests.hpp \
    include/type_ids.hpp \
    src/library_priv.hpp \
    src/util.hpp \

INCLUDEPATH += \
    include/ \
    src/ \

DESTDIR = ../bin/
