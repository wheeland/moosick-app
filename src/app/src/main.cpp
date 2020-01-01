#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>

#include "search.hpp"

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

    Search::Query query("localhost", 8080);

    qRegisterMetaType<Search::BandcampArtistResult*>();
    qRegisterMetaType<Search::BandcampAlbumResult*>();
    qRegisterMetaType<Search::BandcampTrackResult*>();
    qmlRegisterUncreatableType<Search::Result>("Moosick", 1, 0, "Result", "ain't gonna do that from QML!");

    QQuickView view;
    view.resize(screenSize);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
#ifdef Q_OS_ANDROID
    view.rootContext()->setContextProperty("_logger", &logger);
#endif
    view.rootContext()->setContextProperty("_query", &query);
    view.setSource(QUrl("qrc:/qml/main.qml"));
    view.show();

    const int ret = app.exec();

#ifdef Q_OS_ANDROID
    logger.uninstall();
#endif

    return ret;
}
