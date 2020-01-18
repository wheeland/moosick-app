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

static constexpr quint16 DEFAULT_PORT = 12345;

static QString getBackupPath(const QString &libPath)
{
    QFileInfo file(libPath);
    return file.dir().path() + QDir::separator() + file.baseName();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "Specify library file.");
    parser.addOption({{"p", "port"}, "Specify port to listen on.", "port"});
    parser.addOption({{"d", "data"}, "Specify path to library data.", "data", "."});
    parser.addOption({{"l", "log"}, "Specify path to library log.", "log", "."});
    parser.addOption({{"b", "backup"}, "Specify base path for backup files.", "backup", "."});
    parser.process(app);

    const QStringList posArgs = parser.positionalArguments();
    if (posArgs.size() < 1)
        parser.showHelp();

    // get port
    bool portOk;
    quint16 port = parser.value("port").toUShort(&portOk);
    if (!portOk)
        port = DEFAULT_PORT;

    // start TCP server
    const QString libraryPath = posArgs[0];
    const QString dataPath = parser.value("data");
    const QString logPath = parser.isSet("log") ? parser.value("log") : (libraryPath + ".log");
    const QString backupPath = parser.isSet("backup") ? parser.value("backup") : getBackupPath(libraryPath);

    qWarning() << "Data Path =" << dataPath;
    qWarning() << "Log Path =" << logPath;
    qWarning() << "Backup Base Path =" << backupPath;

    Server server(libraryPath, logPath, dataPath, backupPath);
    if (!server.listen(port))
        return 1;

    SignalHandler signalHandler;
    QObject::connect(&signalHandler, &SignalHandler::signalled, &app, &QCoreApplication::quit);

    return app.exec();
}
