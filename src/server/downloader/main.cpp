#include <QCoreApplication>
#include <QCommandLineParser>
#include <QProcess>
#include <QJsonDocument>
#include <QDir>
#include <iostream>

#include "server.hpp"
#include "download.hpp"
#include "jsonconv.hpp"
#include "serversettings.hpp"

static const QString rootDir = qgetenv("DOCUMENT_ROOT");
static const QString toolDir = rootDir + "/../tools/";
static const QString jsDir = toolDir + "scrapers/";
static const QString tmpDir = "/tmp/";
static const QString mediaDir = rootDir;
static const ClientCommon::ServerConfig serverConfig{"localhost", 12345, 1000};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const ServerSettings settings;
    if (!settings.isValid()) {
        qWarning() << "Settings file not valid";
        return 1;
    }

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("server", "Run as server"));
    parser.addOption({QStringList{"download"}, "Download given base64 DownloadRequest", "request", "."});
    parser.process(app);

    const bool isServer = parser.isSet("server");
    const bool isDownload = parser.isSet("download");
    if (isServer == isDownload) {
        qCritical() << "Must be called with either 'server' or 'download'";
        return 1;
    }

    const QString toolDir = settings.toolsDir();
    const QString tmpDir = settings.tempDir();
    const QString mediaDir = settings.serverRoot();

    if (!QDir(toolDir).exists() || !QDir(tmpDir).exists() || !QDir(mediaDir).exists()) {
        qCritical() << "media, tool, and temp directories must exist";
        return 1;
    }

    if (isServer) {
        Server server(argv[0], mediaDir, toolDir, tmpDir, settings.downloaderPort());
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

        return changes.isEmpty() ? 1 : 0;
    }
}
