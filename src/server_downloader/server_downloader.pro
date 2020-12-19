TARGET = downloader
CONFIG += c++11
TEMPLATE = app

QT += core network

SOURCES += \
    main.cpp \
    server.cpp \
    download.cpp \
    \
    ../shared/jsonconv.cpp \
    ../shared/library.cpp \
    ../shared/library_serialize.cpp \
    ../shared/messages.cpp \
    ../shared/requests.cpp \
    ../shared/serversettings.cpp \
    ../shared/tcpserver.cpp \
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
    server.hpp \
    download.hpp \
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
    ../shared/ \
    ../../3rdparty/gumbo-parser/src/ \
    ../../3rdparty/rapidjson/include/ \
    ../../3rdparty/cpp-musicscrape/ \

DESTDIR = ../../bin/
