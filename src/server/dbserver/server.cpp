#include "server.hpp"
#include "messages.hpp"
#include "jsonconv.hpp"

#include <QFile>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QDate>
#include <QProcess>

Server::Server(const QString &libraryPath,
               const QString &logPath,
               const QString &dataPath,
               const QString &backupBasePath)
    : NetCommon::TcpServer()
    , m_libraryPath(libraryPath)
    , m_logPath(logPath)
    , m_dataPath(dataPath)
    , m_backupBasePath(backupBasePath)
{
    // maybe load library file
    QFile libraryFile(libraryPath);
    if (libraryFile.open(QIODevice::ReadOnly)) {
        const QJsonObject jsonRoot = parseJsonObject(libraryFile.readAll(), "Library");
        if (m_library.deserializeFromJson(jsonRoot))
            qDebug() << "Parsed library from" << libraryPath;
        else
            qDebug() << "Failed parsing library from" << libraryPath;
    } else {
        qDebug() << "Could not read file" << libraryPath;
    }

    // check if we can open log file
    if (!QFile(logPath).open(QIODevice::Append)) {
        qWarning() << "Can't write to log file" << logPath;
    }
}

Server::~Server()
{
    saveLibrary();
}

bool Server::handleMessage(const ClientCommon::Message &message, ClientCommon::Message &response)
{
    switch (message.tp) {
    case ClientCommon::Ping: {
        response = {ClientCommon::Pong, {}};
        return true;
    }
    case ClientCommon::ChangesRequest: {
        // read changes from TCP stream
        QDataStream in(message.data);
        QVector<Moosick::LibraryChange> changes;
        in >> changes;

        QVector<Moosick::CommittedLibraryChange> appliedChanges;

        // apply changes
        for (const Moosick::LibraryChange &change : qAsConst(changes)) {
            quint32 newId, result = m_library.commit(change, &newId);
            Moosick::LibraryChange resultChange = change;

            if (result != 0) {
                // if this was an Add change, we'll want to communicate the newly created
                // ID back to the caller
                if (Moosick::LibraryChange::isCreatingNewId(change.changeType))
                    resultChange.detail = newId;

                appliedChanges << Moosick::CommittedLibraryChange{resultChange, result};
            }
        }

        qDebug() << "Applied" << appliedChanges.size() << "changes to Library";

        // send back all successful changes
        response.tp = ClientCommon::ChangesResponse;
        response.data = QJsonDocument(toJson(appliedChanges).toArray()).toJson();

        // append library log
        QFile logFile(m_logPath);
        const qint64 sz = logFile.exists() ? logFile.size() : 0;
        if (logFile.open(QIODevice::Append)) {
            bool first = (sz <= 0);
            for (const auto &ch : qAsConst(appliedChanges)) {
                if (!first)
                    logFile.write(",\n");
                first = false;

                logFile.write(QJsonDocument(toJson(ch).toObject()).toJson().trimmed());
            }
        }

        saveLibrary();

        return true;
    }
    case ClientCommon::LibraryRequest: {
        // send back whole library
        response.tp = ClientCommon::LibraryReponse;
        response.data = QJsonDocument(m_library.serializeToJson()).toJson();
        return true;
    }
    case ClientCommon::ChangeListRequest: {
        const quint32 rev = message.data.toUInt();
        const QVector<Moosick::CommittedLibraryChange> changes = m_library.committedChangesSince(rev);

        response.tp = ClientCommon::ChangeListReponse;
        response.data = QJsonDocument(toJson(changes).toArray()).toJson();
        return true;
    }
    default: {
        response = {ClientCommon::Error, {}};
        return true;
    }
    }
}

void Server::saveLibrary() const
{
    const auto save = [&](const QString &path) {
        QFile libFile(path);
        if (libFile.open(QIODevice::WriteOnly)) {
            const QJsonDocument doc(m_library.serializeToJson());
            libFile.write(doc.toJson());
        }
    };

    save(m_libraryPath);

    // check if back-up for today already exists
    const QDate today = QDate::currentDate();
    const QString dateString = QString::asprintf("%d_%02d_%02d", today.year(), today.month(), today.day());
    const QString backupPath = m_backupBasePath + "." + dateString + ".json";
    if (!QFile::exists(backupPath) && !QFile::exists(backupPath + ".gz")) {
        save(backupPath);
        QProcess::execute("gzip", { backupPath });
    }
}
