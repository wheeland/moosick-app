TARGET = cgi
CONFIG += c++11
TEMPLATE = app

QT += core network

SOURCES += \
    main.cpp \

HEADERS += \

INCLUDEPATH += \
    ../netcommon/ \
    ../clientcommon/include/ \
    ../dbcommon/include/ \

LIBS += \
    -L../bin/ -lclientcommon -lnetcommon -ldbcommon

DESTDIR = ../bin/
