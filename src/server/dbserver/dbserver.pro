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
    ../../common/include/

LIBS += \
    -L../../bin/ -lcommon

DESTDIR = ../../bin/
