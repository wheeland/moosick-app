#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>

#include "controller.hpp"
#include "search.hpp"
#include "playlist.hpp"
#include "database.hpp"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

#ifdef Q_OS_ANDROID
    Logger logger;
    logger.install();
    const QSize screenSize = app.primaryScreen()->size();
#else
    const QSize screenSize = QSize(540, 920);
#endif

    Controller controller;

    qRegisterMetaType<Playlist::Entry*>();
    qRegisterMetaType<Search::BandcampArtistResult*>();
    qRegisterMetaType<Search::BandcampAlbumResult*>();
    qRegisterMetaType<Search::BandcampTrackResult*>();
    qmlRegisterUncreatableType<Search::Result>("Moosick", 1, 0, "SearchResult", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Playlist::Entry>("Moosick", 1, 0, "PlaylistEntry", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbTag>("Moosick", 1, 0, "DbTag", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbArtist>("Moosick", 1, 0, "DbArtist", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbAlbum>("Moosick", 1, 0, "DbAlbum", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbSong>("Moosick", 1, 0, "DbSong", "ain't gonna do that from QML!");

    QQuickView view;
    view.resize(screenSize);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
#ifdef Q_OS_ANDROID
    view.rootContext()->setContextProperty("_logger", &logger);
#endif
    view.rootContext()->setContextProperty("_app", &controller);
    view.setSource(QUrl("qrc:/qml/main.qml"));
    view.show();

    const int ret = app.exec();

#ifdef Q_OS_ANDROID
    logger.uninstall();
#endif

    return ret;
}
