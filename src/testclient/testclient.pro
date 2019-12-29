TARGET = testclient
CONFIG += c++11
TEMPLATE = app

QT += core network

SOURCES += \
    main.cpp \

HEADERS += \

INCLUDEPATH += \
    ../dbcommon/include/ \
    ../clientcommon/include/ \
    ../netcommon/ \

LIBS += \
    -L../bin/ -ldbcommon -lclientcommon -lnetcommon

DESTDIR = ../bin/
