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
    ../dbcommon/include/

LIBS += \
    -L../bin/ -ldbcommon

DESTDIR = ../bin/
