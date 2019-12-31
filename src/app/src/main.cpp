#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QScreen>

#include <QFile>
#include <QDir>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

#ifdef Q_OS_ANDROID
    Logger logger;
    logger.install();
#endif


    return 0;
}
