TARGET = uploader
CONFIG += c++11
TEMPLATE = app

QT += gui widgets core network

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    connection.cpp \
    connectiondialog.cpp \
    fileview.cpp \
    \
    ../shared/jsonconv.cpp \
    ../shared/library.cpp \
    ../shared/library_serialize.cpp \

HEADERS += \
    mainwindow.hpp \
    connection.hpp \
    connectiondialog.hpp \
    fileview.hpp \
    \
    ../shared/flatmap.hpp \
    ../shared/jsonconv.hpp \
    ../shared/library.hpp \
    ../shared/library_types.hpp \
    ../shared/library_messages.hpp \
    ../shared/result.hpp \
    ../shared/option.hpp \

LIBS += \
    -ltag

INCLUDEPATH += \
    ../shared/ \

DESTDIR = ../bin/
