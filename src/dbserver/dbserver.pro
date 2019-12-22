TARGET = dbserver
CONFIG += c++11
TEMPLATE = app

QT += core network

SOURCES += \
    main.cpp \
    server.cpp \

HEADERS += \
    server.hpp \

INCLUDEPATH += \
    ../dbcommon/include/ \
    ../netcommon/ \

LIBS += \
    -L../bin/ -ldbcommon -lnetcommon

DESTDIR = ../bin/
