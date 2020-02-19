TARGET = testclient
CONFIG += c++11
TEMPLATE = app

QT -= gui
QT += core network

SOURCES += \
    main.cpp \

HEADERS += \

INCLUDEPATH += \
    ../common/include/

LIBS += \
    -L../bin/ -lcommon

DESTDIR = ../bin/
