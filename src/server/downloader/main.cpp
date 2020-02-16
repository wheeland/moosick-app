#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QJsonDocument>
#include <QDir>
#include <iostream>

#include "server.hpp"
#include "download.hpp"
#include "jsonconv.hpp"

static const QString rootDir = qgetenv("DOCUMENT_ROOT");
static const QString toolDir = rootDir + "/../tools/";
static const QString jsDir = toolDir + "scrapers/";
static const QString tmpDir = "/tmp/";
static const QString mediaDir = rootDir;
static const ClientCommon::ServerConfig serverConfig{"localhost", 12345, 1000};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({QStringList{"server"}, "Run as server, specify port", "port"});
    parser.addOption({QStringList{"download"}, "Download given base64 DownloadRequest", "request", "."});
    parser.addOption({QStringList{"media"}, "Media directory", "media"});
    parser.addOption({QStringList{"tool"}, "Tool directory", "tool"});
    parser.addOption({QStringList{"temp"}, "Temp directory", "temp"});
    parser.process(app);

    const bool isServer = parser.isSet("server");
    const bool isDownload = parser.isSet("download");
    if (isServer == isDownload) {
        qCritical() << "Must be called with either 'server' or 'download'";
        return 1;
    }

    if (!parser.isSet("media") || !parser.isSet("tool") || !parser.isSet("temp")) {
        qCritical() << "media, tool, and temp directories must be set";
        return 1;
    }

    const QString toolDir = parser.value("tool");
    const QString tmpDir = parser.value("temp");
    const QString mediaDir = parser.value("media");

    if (!QDir(toolDir).exists() || !QDir(tmpDir).exists() || !QDir(mediaDir).exists()) {
        qCritical() << "media, tool, and temp directories must exist";
        return 1;
    }

    if (isServer) {
        const quint16 port = parser.value("server").toInt();
        if (port == 0) {
            qCritical() << parser.value("server") << "is not a valid port";
            return 1;
        }

        Server server(argv[0], mediaDir, toolDir, tmpDir, port);
        return app.exec();
    }

    if (isDownload) {
        NetCommon::DownloadRequest request;
        const QString base64 = parser.value("download");
        if (!request.fromBase64(base64.toLocal8Bit())) {
            qCritical() << "Failed parsing the base64 download request";
            return 1;
        }

        const QVector<Moosick::CommittedLibraryChange> changes = ClientCommon::download(
                    serverConfig, request, mediaDir, toolDir, tmpDir);

        std::cout << QJsonDocument(toJson(changes).toArray()).toJson().constData() << std::endl;

        return 0;
    }
}
