TARGET = dbserver
CONFIG += c++11
TEMPLATE = app

QT += core network

SOURCES += \
    main.cpp \
    server.cpp \
    signalhandler.cpp \

HEADERS += \
    server.hpp \
    signalhandler.hpp \

INCLUDEPATH += \
    ../dbcommon/include/ \
    ../clientcommon/ \

LIBS += \
    -L../bin/ -ldbcommon -lclientcommon

DESTDIR = ../bin/
