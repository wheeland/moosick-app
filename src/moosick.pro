TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = \
    common \
    app \

!android {
    SUBDIRS += server testclient
}
