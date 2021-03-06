#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>

#include "controller.hpp"
#include "search.hpp"
#include "playlist.hpp"
#include "database/database.hpp"
#include "stringmodel.hpp"
#include "selecttagsmodel.hpp"
#include "multichoicecontroller.hpp"
#include "util/androidutil.hpp"

#ifdef Q_OS_ANDROID
static bool isService(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == QByteArray("--service"))
            return true;
    }
    return false;
}
#endif // Q_OS_ANDROID

int main(int argc, char **argv)
{
#ifndef Q_OS_ANDROID
    // only use QtVirtualKeyboard on host builds, doesn't seem to work on Android
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    qputenv("QT_VIRTUALKEYBOARD_LAYOUT_PATH", QByteArray(":/qml/layouts/"));
#endif // Q_OS_ANDROID

#ifdef Q_OS_ANDROID
    if (isService(argc, argv))
        return runPlaybackService(argc, argv);
#endif // Q_OS_ANDROID

    QCoreApplication::setOrganizationName("de.wheeland.moosick");
    QCoreApplication::setApplicationName("Moosick");

    QGuiApplication app(argc, argv);

#ifdef Q_OS_ANDROID
    startPlaybackService();
#endif // Q_OS_ANDROID

    AndroidUtil::Logger logger;
    logger.install();

    static const int DEFAULT_WIDTH = 540;

#ifdef Q_OS_ANDROID
    const QSize screenSize = app.primaryScreen()->size();
#else
    const QSize screenSize = QSize(450, 750);
#endif

    Controller controller;

    qRegisterMetaType<Playlist::Entry*>();
    qRegisterMetaType<Search::BandcampArtistResult*>();
    qRegisterMetaType<Search::BandcampAlbumResult*>();
    qRegisterMetaType<Search::BandcampTrackResult*>();
    qmlRegisterUncreatableType<Search::Query>("Moosick", 1, 0, "SearchQuery", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Search::Result>("Moosick", 1, 0, "SearchResult", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Playlist::Playlist>("Moosick", 1, 0, "Playlist", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Playlist::Entry>("Moosick", 1, 0, "PlaylistEntry", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DatabaseInterface>("Moosick", 1, 0, "Database", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbTag>("Moosick", 1, 0, "DbTag", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbArtist>("Moosick", 1, 0, "DbArtist", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbAlbum>("Moosick", 1, 0, "DbAlbum", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<Database::DbSong>("Moosick", 1, 0, "DbSong", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<StringModel>("Moosick", 1, 0, "StringModel", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<SelectTagsModel>("Moosick", 1, 0, "SelectTagsModel", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<MultiChoiceController>("Moosick", 1, 0, "MultiChoiceController", "ain't gonna do that from QML!");
    qmlRegisterUncreatableType<HttpClient>("Moosick", 1, 0, "HttpClient", "ain't gonna do that from QML!");

    MultiChoiceController multiChoiceController;

    QQuickView view;
    view.resize(screenSize);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.rootContext()->setContextProperty("_logger", &logger);
#ifdef Q_OS_ANDROID
    view.rootContext()->setContextProperty("_isAndroid", true);
#else
    view.rootContext()->setContextProperty("_isAndroid", false);
#endif
    view.rootContext()->setContextProperty("_app", &controller);
    view.rootContext()->setContextProperty("_defaultWindowWidth", DEFAULT_WIDTH);
    view.rootContext()->setContextProperty("_multiChoiceController", &multiChoiceController);

    QQmlComponent styleComponent(view.engine(), "qrc:/qml/Style.qml");
    QObject *styleObject = styleComponent.create(view.rootContext());
    view.rootContext()->setContextProperty("_style", styleObject);

    view.setSource(QUrl("qrc:/qml/main.qml"));
    view.show();

    qWarning() << "Started";

    const int ret = app.exec();
    logger.uninstall();
    return ret;
}
