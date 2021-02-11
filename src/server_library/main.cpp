#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QDir>

#include <QTcpServer>
#include <QTcpSocket>

#include "library.hpp"
#include "server.hpp"
#include "signalhandler.hpp"
#include "serversettings.hpp"
#include "logger.hpp"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const ServerSettings settings;
    if (!settings.isValid()) {
        qCritical() << "Settings file not valid";
        return 1;
    }

    if (!settings.dbserverLogFile().isEmpty()) {
        Logger::setLogFile(settings.dbserverLogFile());
        Logger::install();
    }

    // start TCP server
    const QString libraryPath = settings.libraryFile();
    const QString logPath = settings.libraryLogFile();
    const QString backupPath = settings.libraryBackupDir();

    qInfo() << "Library File =" << libraryPath;
    qInfo() << "Log Path =" << logPath;
    qInfo() << "Backup Base Path =" << backupPath;

    Server server;
    JsonifyError error = server.init(libraryPath, logPath, backupPath);
    if (error.isError()) {
        qCritical().noquote() << "Failed to initialize server:" << error.toString();
        return 1;
    }
    if (!server.listen(settings.dbserverPort()))
        return 1;

    SignalHandler signalHandler;
    QObject::connect(&signalHandler, &SignalHandler::signalled, &app, &QCoreApplication::quit);

    return app.exec();
}
