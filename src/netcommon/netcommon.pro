TARGET = netcommon
CONFIG += c++11
TEMPLATE = lib

QT += core network

SOURCES += \
    requests.cpp \
    json.cpp \

HEADERS += \
    requests.hpp \
    json.hpp \

INCLUDEPATH += \
    ../dbcommon/include/

LIBS += \
    -L../bin/ -ldbcommon

DESTDIR = ../bin/
