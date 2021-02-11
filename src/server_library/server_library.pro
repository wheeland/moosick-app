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
    ../shared/logger.cpp \
    ../shared/serversettings.cpp \
    ../shared/tcpclientserver.cpp \

HEADERS += \
    server.hpp \
    signalhandler.hpp \
    \
    ../shared/flatmap.hpp \
    ../shared/jsonconv.hpp \
    ../shared/library.hpp \
    ../shared/library_types.hpp \
    ../shared/library_messages.hpp \
    ../shared/logger.hpp \
    ../shared/serversettings.hpp \
    ../shared/tcpclientserver.hpp \

INCLUDEPATH += \
    ../shared/

DESTDIR = ../../bin/
