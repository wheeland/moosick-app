TEMPLATE = subdirs

SUBDIRS = app

!android {
    SUBDIRS += \
        server_cgi \
        server_library \
        server_downloader \
        #testclient \
}
