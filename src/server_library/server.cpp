#include "server.hpp"
#include "jsonconv.hpp"
#include "download.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QDate>
#include <QProcess>
#include <QThread>

using namespace Moosick;
using namespace MoosickMessage;

class DownloaderThread : public QThread
{
    Q_OBJECT

public:
    DownloaderThread(const MoosickMessage::DownloadRequest &request, Server *server)
        : m_request(request)
        , m_server(server)
    {}
    ~DownloaderThread() {}

    void run() override
    {
        const QString tmpDir = m_server->m_settings.tempDir();
        const QString toolDir = m_server->m_settings.toolsDir();

        m_result = download(m_request, toolDir, tmpDir);
    }

private:
    MoosickMessage::DownloadRequest m_request;
    Server *m_server;
    Result<DownloadResult, QString> m_result;

    friend class Server;
};

static Result<SerializedLibrary, EnjsonError> loadLibraryJson(const QString &libraryPath)
{
    QFile libraryFile(libraryPath);
    if (!libraryFile.open(QIODevice::ReadOnly))
        return EnjsonError::buildCustomError("Can't open file");

    return dejsonFromString<SerializedLibrary>(libraryFile.readAll());
}

static Result<QJsonArray, EnjsonError> loadLibraryLogJson(const QString &logPath)
{
    QFile libraryLogFile(logPath);
    if (!libraryLogFile.open(QIODevice::ReadOnly))
        return EnjsonError::buildCustomError("Can't open file");

    const QByteArray logData = "[\n" + libraryLogFile.readAll() + "\n]\n";
    return jsonDeserializeArray(logData);
}

QString Server::createSongHandleFile(SongId songId)
{
    QString dstFileName;
    do {
        Moosick::SongHandle handle = Moosick::SongHandle::generate();
        m_library.commit(LibraryChangeRequest::CreateSongSetHandle(songId, 0, QString::fromUtf8(handle.toString())));
        dstFileName = m_settings.mediaBaseDir() + QDir::separator() + songId.fileName(m_library);
    } while (QFile::exists(dstFileName));
    return dstFileName;
}

Server::Server()
    : TcpServer()
{
}

EnjsonError Server::init(const ServerSettings &settings)
{
    m_settings = settings;

    const QString libraryPath = settings.libraryFile();
    const QString logPath = settings.libraryLogFile();

    const bool libraryExists = QFile::exists(libraryPath);
    const bool logExists = QFile::exists(logPath);

    if (logExists && !libraryExists)
        return EnjsonError::buildCustomError("Log file exists, but library file doesn't");
    if (!logExists && libraryExists)
        return EnjsonError::buildCustomError("Library file exists, but log file doesn't");

    if (!libraryExists) {
        if (!QFile(libraryPath).open(QIODevice::WriteOnly))
            return EnjsonError::buildCustomError("Failed to create library file");
        if (!QFile(logPath).open(QIODevice::WriteOnly))
            return EnjsonError::buildCustomError("Failed to create log file");
        saveLibrary();
        qWarning() << "Library doesn't yet exist, creating new one";
        return {};
    }

    Result<SerializedLibrary, EnjsonError> libraryJson = loadLibraryJson(libraryPath);
    if (!libraryJson.hasValue())
        return EnjsonError::buildCustomError(QString("Failed to load ") + libraryPath, libraryJson.takeError());

    Result<QJsonArray, EnjsonError> logJson = loadLibraryLogJson(logPath);
    if (!logJson.hasValue())
        return EnjsonError::buildCustomError(QString("Failed to load ") + logPath, logJson.takeError());

    // check if we can open log file
    if (!QFile(logPath).open(QIODevice::Append))
        return EnjsonError::buildCustomError(QString("Failed to open ") + logPath + " for writing");

    EnjsonError result = m_library.deserializeFromJson(libraryJson.takeValue(), logJson.takeValue());
    if (result.isError())
        return EnjsonError::buildCustomError("Error while parsing library", result);

    return EnjsonError();
}

Server::~Server()
{
    saveLibrary();
}

QByteArray Server::handleMessage(const QByteArray &data)
{
    Result<Message, EnjsonError> messageParsingResult = Message::fromJson(data);
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
        for (const LibraryChangeRequest &change : *changesRequest->changes) {
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
        QFile logFile(m_settings.libraryLogFile());
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
    case Type::UploadSongRequestInternal: {
        const UploadSongRequestInternal *uploadSongRequest = message.as<UploadSongRequestInternal>();
        if (!QFileInfo(uploadSongRequest->filePath).isReadable())
            return messageToJson(Error("Internal error"));

        const ArtistId artist = getOrCreateArtist(uploadSongRequest->artistName);
        const AlbumId album = getOrCreateAlbum(artist, uploadSongRequest->albumName);
        const SongId songId = m_library.commit(LibraryChangeRequest::CreateSongAdd(album, 0, ""))
                        .mapMember<SongId>(&CommittedLibraryChange::createdId)
                        .takeValue();

        m_library.commit(LibraryChangeRequest::CreateSongSetFileEnding(songId, 0, uploadSongRequest->fileEnding));
        m_library.commit(LibraryChangeRequest::CreateSongSetName(songId, 0, uploadSongRequest->title));
        m_library.commit(LibraryChangeRequest::CreateSongSetLength(songId, uploadSongRequest->duration));
        m_library.commit(LibraryChangeRequest::CreateSongSetPosition(songId, uploadSongRequest->position));

        // assign handle and move to destination
        QFile(uploadSongRequest->filePath).rename(createSongHandleFile(songId));

        UploadSongResponse response;
        response.songId = songId;
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
    case Type::DownloadRequest: {
        const DownloadRequest *downloadRequest = message.as<DownloadRequest>();
        const quint32 id = startDownload(*downloadRequest);
        DownloadResponse response;
        response.downloadId = id;
        return messageToJson(response);
    }
    case Type::DownloadQuery: {
        DownloadQueryResponse response;
        for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it)
            response.activeRequests->append(DownloadQueryResponse::ActiveDownload{ it.key(), it.value().request });
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

    save(m_settings.libraryFile());

    // check if back-ups for today already exists
    const QDate today = QDate::currentDate();
    const QString dateString = QString::asprintf("%d_%02d_%02d", today.year(), today.month(), today.day());

    // backup library
    const QString backupPath = m_settings.libraryBackupDir() + "." + dateString + ".json";
    if (!QFile::exists(backupPath) && !QFile::exists(backupPath + ".gz")) {
        save(backupPath);
        QProcess::execute("gzip", { backupPath });
    }

    // backup log
    const QString logBackupPath = m_settings.libraryBackupDir() + "." + dateString + ".log.json";
    if (!QFile::exists(backupPath) && !QFile::exists(backupPath + ".gz")) {
        QFile(m_settings.libraryLogFile()).copy(logBackupPath);
        QProcess::execute("gzip", { logBackupPath });
    }
}

ArtistId Server::getOrCreateArtist(const QString &name)
{
    const QVector<ArtistId> artists = m_library.artistsByName();
    for (ArtistId artist : artists) {
        if (artist.name(m_library) == name)
            return artist;
    }

    return m_library.commit(LibraryChangeRequest::CreateArtistAdd(0, 0, name))
            .mapMember<ArtistId>(&CommittedLibraryChange::createdId)
            .takeValue();
}

AlbumId Server::getOrCreateAlbum(ArtistId artist, const QString &name)
{
    const QVector<AlbumId> albums = artist.albums(m_library);
    for (AlbumId album : albums) {
        if (album.name(m_library) == name)
            return album;
    }

    return m_library.commit(LibraryChangeRequest::CreateAlbumAdd(artist, 0, name))
            .mapMember<AlbumId>(&CommittedLibraryChange::createdId)
            .takeValue();
}

quint32 Server::startDownload(const MoosickMessage::DownloadRequest &request)
{
    const quint32 id = m_nextDownloadId++;

    qDebug() << "Starting download" << id << ":" << (int) *request.requestType << *request.url << *request.albumName << *request.artistId << *request.artistName;

    DownloaderThread *thread = new DownloaderThread(request, this);
    m_downloads[id] = RunningDownload { request, thread };
    connect(thread, &DownloaderThread::finished, this, [=]() {
        onDownloaderThreadFinished(thread);
    });

    thread->start();

    return id;
}

void Server::onDownloaderThreadFinished(DownloaderThread *thread)
{
    quint32 id = 0;
    for (auto it = m_downloads.begin(); it != m_downloads.end(); ++it) {
        if (it->thread == thread) {
            id = it.key();
            m_downloads.erase(it);
            break;
        }
    }

    if (thread->m_result.hasError()) {
        qWarning() << "Failed to download:" << thread->m_result.getError();
    }
    else {
        finishDownload(id, thread->m_result.takeValue());
    }

    thread->wait();
    delete thread;
}

void Server::finishDownload(quint32 id, const DownloadResult &result)
{
    // 1. Get or create artist
    ArtistId artistId = result.artistId;
    if (!artistId.isValid() || !artistId.exists(m_library))
        artistId = getOrCreateArtist(result.artistName);

    // 2. Create album
    const AlbumId albumId = getOrCreateAlbum(artistId, result.albumName);

    // 3. Add songs
    for (const DownloadResult::File &file : result.files)
    {
        const SongId songId = m_library.commit(LibraryChangeRequest::CreateSongAdd(albumId, 0, file.title))
                .mapMember<SongId>(&CommittedLibraryChange::createdId)
                .takeValue();

        m_library.commit(LibraryChangeRequest::CreateSongSetName(songId, 0, file.title));
        m_library.commit(LibraryChangeRequest::CreateSongSetPosition(songId, file.albumPosition));
        m_library.commit(LibraryChangeRequest::CreateSongSetLength(songId, file.duration));
        m_library.commit(LibraryChangeRequest::CreateSongSetFileEnding(songId, 0, file.fileEnding));

        // assign handle and move to destination
        QFile(file.fullPath).rename(createSongHandleFile(songId));
    }

    // 4. Remove temp dir
    if (!result.tempDir.isEmpty() && QDir().exists(result.tempDir))
        QDir().rmdir(result.tempDir);

    qDebug() << "Finished download" << id << ":" << artistId.name(m_library) << albumId.name(m_library) << result.files.size() << "songs";
}

#include "server.moc"
