#include "server.hpp"
#include "jsonconv.hpp"

#include <QFile>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QDate>
#include <QProcess>

using namespace Moosick;
using namespace MoosickMessage;

static Result<SerializedLibrary, JsonifyError> loadLibraryJson(const QString &libraryPath)
{
    QFile libraryFile(libraryPath);
    if (!libraryFile.open(QIODevice::ReadOnly))
        return JsonifyError::buildCustomError("Can't open file");

    return dejsonFromString<SerializedLibrary>(libraryFile.readAll());
}

static Result<QJsonArray, JsonifyError> loadLibraryLogJson(const QString &logPath)
{
    QFile libraryLogFile(logPath);
    if (!libraryLogFile.open(QIODevice::ReadOnly))
        return JsonifyError::buildCustomError("Can't open file");

    const QByteArray logData = "[\n" + libraryLogFile.readAll() + "\n]\n";
    return jsonDeserializeArray(logData);
}

Server::Server()
    : TcpServer()
{
}

JsonifyError Server::init(const QString &libraryPath,
                          const QString &logPath,
                          const QString &backupBasePath)
{
    m_libraryPath = libraryPath;
    m_logPath = logPath;
    m_backupBasePath = backupBasePath;

    const bool libraryExists = QFile::exists(libraryPath);
    const bool logExists = QFile::exists(logPath);

    if (logExists && !libraryExists)
        return JsonifyError::buildCustomError("Log file exists, but library file doesn't");
    if (!logExists && libraryExists)
        return JsonifyError::buildCustomError("Library file exists, but log file doesn't");

    if (!libraryExists) {
        if (!QFile(libraryPath).open(QIODevice::WriteOnly))
            return JsonifyError::buildCustomError("Failed to create library file");
        if (!QFile(logPath).open(QIODevice::WriteOnly))
            return JsonifyError::buildCustomError("Failed to create log file");
        saveLibrary();
        qWarning() << "Library doesn't yet exist, creating new one";
        return {};
    }

    Result<SerializedLibrary, JsonifyError> libraryJson = loadLibraryJson(libraryPath);
    if (!libraryJson.hasValue())
        return JsonifyError::buildCustomError(QString("Failed to load ") + libraryPath, libraryJson.takeError());

    Result<QJsonArray, JsonifyError> logJson = loadLibraryLogJson(logPath);
    if (!logJson.hasValue())
        return JsonifyError::buildCustomError(QString("Failed to load ") + logPath, logJson.takeError());

    // check if we can open log file
    if (!QFile(logPath).open(QIODevice::Append))
        return JsonifyError::buildCustomError(QString("Failed to open ") + logPath + " for writing");

    JsonifyError result = m_library.deserializeFromJson(libraryJson.takeValue(), logJson.takeValue());
    if (result.isError())
        return JsonifyError::buildCustomError("Error while parsing library", result);

    return JsonifyError();
}

Server::~Server()
{
    saveLibrary();
}

QByteArray Server::handleMessage(const QByteArray &data)
{
    Result<Message, JsonifyError> messageParsingResult = Message::fromJson(data);
    if (messageParsingResult.hasError()) {
        qWarning().noquote() << "Error parsing message:" << messageParsingResult.takeError().toString();
        return QByteArray();
    }

    Message message = messageParsingResult.takeValue();

    switch (message.getType()) {
    case Type::Ping: {
        return messageToJson(Pong());
    }
    case Type::ChangesRequest: {
        const ChangesRequest *changesRequest = message.as<ChangesRequest>();
        QVector<CommittedLibraryChange> appliedChanges;

        // apply changes
        for (const LibraryChangeRequest &change : changesRequest->changes.data()) {
            Result<CommittedLibraryChange, QString> committed = m_library.commit(change);
            if (committed.hasError()) {
                qWarning().noquote() << "Error applying changes:" << committed.takeError();
            }
            else {
                appliedChanges << committed.takeValue();
            }
        }

        qDebug() << "Applied" << appliedChanges.size() << "changes to Library";

        // append library log
        QFile logFile(m_logPath);
        const qint64 sz = logFile.exists() ? logFile.size() : 0;
        if (logFile.open(QIODevice::Append)) {
            bool first = (sz <= 0);
            for (const auto &ch : qAsConst(appliedChanges)) {
                if (!first)
                    logFile.write(",\n");
                first = false;

                logFile.write(jsonSerializeObject(enjson(ch).toObject()).trimmed());
            }
        }

        saveLibrary();

        // send back all successful changes
        ChangesResponse response;
        response.changes = appliedChanges;
        return messageToJson(response);
    }
    case Type::LibraryRequest: {
        LibraryResponse response;
        SerializedLibrary serialized = m_library.serializeToJson();
        response.libraryJson = serialized.libraryJson;
        response.version = serialized.version;
        return messageToJson(response);
    }
    case Type::IdRequest: {
        IdResponse response;
        response.id = QString::fromUtf8(m_library.id().toString());
        return messageToJson(response);
    }
    case Type::ChangeListRequest: {
        const ChangeListRequest *changeListRequest = message.as<ChangeListRequest>();
        const quint32 rev = changeListRequest->revision;
        const QVector<CommittedLibraryChange> changes = m_library.committedChangesSince(rev);
        ChangeListResponse response;
        response.changes = changes;
        return messageToJson(response);
    }
    default: {
        return messageToJson(Error());
    }
    }
}

void Server::saveLibrary() const
{
    const auto save = [&](const QString &path) {
        QFile libFile(path);
        if (libFile.open(QIODevice::WriteOnly)) {
            SerializedLibrary json = m_library.serializeToJson();
            libFile.write(enjsonToString(json));
        }
    };

    save(m_libraryPath);

    // check if back-ups for today already exists
    const QDate today = QDate::currentDate();
    const QString dateString = QString::asprintf("%d_%02d_%02d", today.year(), today.month(), today.day());

    // backup library
    const QString backupPath = m_backupBasePath + "." + dateString + ".json";
    if (!QFile::exists(backupPath) && !QFile::exists(backupPath + ".gz")) {
        save(backupPath);
        QProcess::execute("gzip", { backupPath });
    }

    // backup log
    const QString logBackupPath = m_backupBasePath + "." + dateString + ".log.json";
    if (!QFile::exists(backupPath) && !QFile::exists(backupPath + ".gz")) {
        QFile(m_logPath).copy(logBackupPath);
        QProcess::execute("gzip", { logBackupPath });
    }
}
