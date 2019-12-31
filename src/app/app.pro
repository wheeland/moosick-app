TARGET = moosick
CONFIG += c++11
TEMPLATE = app

QT += core quick qml network

SOURCES += \
    src/main.cpp \
    src/search.cpp \
    src/util/androidutil.cpp \

HEADERS += \
    src/search.hpp \
    src/util/androidutil.hpp \

RESOURCES += \
    qml.qrc

INCLUDEPATH += \
    ../dbcommon/include/ \
    ../netcommon/ \

LIBS += \
    -L../bin/ -ldbcommon -lnetcommon

DESTDIR = ../bin/
