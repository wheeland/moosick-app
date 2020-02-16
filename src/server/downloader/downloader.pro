TARGET = downloader
CONFIG += c++11
TEMPLATE = app

QT += core network

SOURCES += \
    main.cpp \
    server.cpp

HEADERS += \
    server.hpp

INCLUDEPATH += \
    ../../common/include/

LIBS += \
    -L../../bin/ -lcommon

DESTDIR = ../../bin/
