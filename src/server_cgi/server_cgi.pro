TARGET = cgi
CONFIG += c++11
TEMPLATE = app

QT -= gui
QT += core network

SOURCES += \
    main.cpp \
    \
    ../shared/jsonconv.cpp \
    ../shared/library.cpp \
    ../shared/library_serialize.cpp \
    ../shared/logger.cpp \
    ../shared/serversettings.cpp \
    ../shared/tcpclientserver.cpp \
    \
    ../../3rdparty/gumbo-parser/src/attribute.c \
    ../../3rdparty/gumbo-parser/src/char_ref.c \
    ../../3rdparty/gumbo-parser/src/error.c \
    ../../3rdparty/gumbo-parser/src/parser.c \
    ../../3rdparty/gumbo-parser/src/string_buffer.c \
    ../../3rdparty/gumbo-parser/src/string_piece.c \
    ../../3rdparty/gumbo-parser/src/tag.c \
    ../../3rdparty/gumbo-parser/src/tokenizer.c \
    ../../3rdparty/gumbo-parser/src/utf8.c \
    ../../3rdparty/gumbo-parser/src/util.c \
    ../../3rdparty/gumbo-parser/src/vector.c \
    \
    ../../3rdparty/cpp-musicscrape/musicscrape/musicscrape.cpp \

HEADERS += \
    ../shared/flatmap.hpp \
    ../shared/jsonconv.hpp \
    ../shared/library.hpp \
    ../shared/library_types.hpp \
    ../shared/library_messages.hpp \
    ../shared/logger.hpp \
    ../shared/serversettings.hpp \
    ../shared/tcpclientserver.hpp \

INCLUDEPATH += \
    ../shared/ \
    ../../3rdparty/gumbo-parser/src/ \
    ../../3rdparty/rapidjson/include/ \

DESTDIR = ../bin/
