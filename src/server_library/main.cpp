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

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const ServerSettings settings;
    if (!settings.isValid()) {
        qWarning() << "Settings file not valid";
        return 1;
    }

    // start TCP server
    const QString libraryPath = settings.libraryFile();
    const QString logPath = settings.libraryLogFile();
    const QString backupPath = settings.libraryBackupDir();

    qWarning() << "Library File =" << libraryPath;
    qWarning() << "Log Path =" << logPath;
    qWarning() << "Backup Base Path =" << backupPath;

    Server server(libraryPath, logPath, backupPath);
    if (!server.listen(settings.dbserverPort()))
        return 1;

    SignalHandler signalHandler;
    QObject::connect(&signalHandler, &SignalHandler::signalled, &app, &QCoreApplication::quit);

    return app.exec();
}
