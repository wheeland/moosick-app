TARGET = dbserver
CONFIG += c++11
TEMPLATE = app

QT -= gui
QT += core network

SOURCES += \
    main.cpp \
    server.cpp \
    signalhandler.cpp \
    \
    ../shared/jsonconv.cpp \
    ../shared/library.cpp \
    ../shared/library_serialize.cpp \
    ../shared/messages.cpp \
    ../shared/requests.cpp \
    ../shared/serversettings.cpp \
    ../shared/tcpserver.cpp \

HEADERS += \
    server.hpp \
    signalhandler.hpp \
    \
    ../shared/flatmap.hpp \
    ../shared/jsonconv.hpp \
    ../shared/library.hpp \
    ../shared/library_types.hpp \
    ../shared/messages.hpp \
    ../shared/requests.hpp \
    ../shared/serversettings.hpp \
    ../shared/tcpserver.hpp \

INCLUDEPATH += \
    ../shared/

DESTDIR = ../../bin/
