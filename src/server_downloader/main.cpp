#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QJsonDocument>
#include <QDir>
#include <QDebug>

#include <iostream>
#include <string>

#include "server.hpp"
#include "download.hpp"
#include "jsonconv.hpp"
#include "serversettings.hpp"
#include "logger.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const ServerSettings settings;
    if (!settings.isValid()) {
        qWarning() << "Settings file not valid";
        return 1;
    }

    if (!settings.downloaderLogFile().isEmpty()) {
        Logger::setLogFile(settings.downloaderLogFile());
        Logger::install();
    }

    const QString toolDir = settings.toolsDir();
    const QString tmpDir = settings.tempDir();
    const QString mediaDir = settings.mediaBaseDir();

    if (!QDir(toolDir).exists() || !QDir(tmpDir).exists() || !QDir(mediaDir).exists()) {
        qCritical() << "media, tool, and temp directories must exist";
        return 1;
    }

    Server server(settings);
    return app.exec();
}
