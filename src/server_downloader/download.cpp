#include "download.hpp"
#include "jsonconv.hpp"
#include "tcpclientserver.hpp"
#include "option.hpp"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

#include "musicscrape/musicscrape.hpp"

using namespace Moosick;
using namespace MoosickMessage;

class ChangeSet
{
public:
    ChangeSet(QTcpSocket &socket)
        : m_socket(socket)
    {
    }

    Option<QString> error() const { return m_error; }
    QVector<CommittedLibraryChange> changes() const { return m_committedChanges; }

    /**
     * Returns true on success, false on failure. On failure, the last error can be
     * retrieved using lastError()
     */
    Result<QVector<CommittedLibraryChange>, QString> request(const QVector<LibraryChangeRequest> &changes)
    {
        if (m_error)
            return m_error.getValue();

        #define ERROR_IF(condition, message) do { if (condition) { m_error = message; return m_error.getValue(); } } while (0)

        ChangesRequest request;
        request.changes = changes;
        const QByteArray requestJson = Message(request).toJson();

        Result<QByteArray, QString> result = TcpClient::sendMessage(m_socket, requestJson, 1000);
        ERROR_IF(result.hasError(), result.takeError());

        Result<Message, JsonifyError> answer = Message::fromJson(result.takeValue());
        ERROR_IF(answer.hasError(), answer.takeError().toString());

        const ChangesResponse *response = answer.getValue().as<ChangesResponse>();
        ERROR_IF(!response, "Received invalid message type");

        m_committedChanges << response->changes.data();
        ERROR_IF(response->changes->size() != changes.size(), "Response changes don't match request");

        for (int i = 0; i < response->changes->size(); ++i) {
            const LibraryChangeRequest requested = changes[i];
            const CommittedLibraryChange committed = response->changes->at(i);
            ERROR_IF(committed.changeRequest.changeType != requested.changeType, "Response changes don't match request");
            if (committed.changeRequest.changeType == LibraryChangeRequest::ArtistAdd
                    || committed.changeRequest.changeType == LibraryChangeRequest::AlbumAdd
                    || committed.changeRequest.changeType == LibraryChangeRequest::SongAdd
                    || committed.changeRequest.changeType == LibraryChangeRequest::TagAdd)
            {
                ERROR_IF(committed.createdId == 0, "No ID was created");
            }
        }

        #undef ERROR_IF

        return response->changes.data();
    }

private:
    QTcpSocket &m_socket;
    QVector<CommittedLibraryChange> m_committedChanges;
    Option<QString> m_error;
};

static int runProcess(
        const QString &program,
        const QStringList &args,
        QByteArray *out,
        QByteArray *err,
        int timeout,
        const QByteArray &inputData = QByteArray())
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(args);
    process.start();

    process.write(inputData);

    int retCode = 0;

    if (!process.waitForFinished(timeout)) {
        qWarning() << "Process" << program << "didn't finish:" << process.error() << process.state() << process.exitStatus();
        retCode = -1;
    } else {
        retCode = process.exitCode();
    }

    if (out) {
        out->clear();
        *out = process.readAllStandardOutput();
    }
    if (err) {
        err->clear();
        *err= process.readAllStandardError();
    }

    return retCode;
}

static QString createTempDir(const QString &dir)
{
    const bool result = QDir().mkpath(dir);
    Q_ASSERT(result);

    QByteArray out;
    runProcess("mktemp", {"-d", "-p", dir}, &out, nullptr, 1000);
    return QString(out).trimmed();
}

static QString createTempFile(const QString &dir, const QString &suffix = QString())
{
    const bool result = QDir().mkpath(dir);
    Q_ASSERT(result);

    QByteArray out;
    runProcess("tempfile", {"-d", dir, "-s", suffix}, &out, nullptr, 1000);
    return QString(out).trimmed();
}

struct YoutubeChapter
{
    QString title;
    int startTime;
    int endTime;
};

struct YoutubeInfo
{
    QString title;
    int duration;
    QVector<YoutubeChapter> chapters;
};

YoutubeInfo parseInfo(const QString &url, const QString &toolDir)
{
    const QString youtubeId = url.split("=").last();
    QByteArray out, err;
    runProcess(toolDir + "/youtube-dl", {"-j", url}, &out, &err, 60000);

    const QJsonDocument doc = QJsonDocument::fromJson(out);
    const QJsonObject root = doc.object();

    YoutubeInfo ret;
    ret.title = root.value("title").toString();
    ret.duration = root.value("duration").toInt();
    for (const QJsonValue &chapterVal : root.value("chapters").toArray()) {
        const QJsonObject obj = chapterVal.toObject();
        const YoutubeChapter chapter {
            obj.value("title").toString(),
            obj.value("start_time").toInt(),
            obj.value("end_time").toInt(),
        };
        if (!chapter.title.isEmpty() && chapter.startTime >= 0 && chapter.endTime > 0) {
            ret.chapters << chapter;
        }
    }
    return ret;
}

Result<QVector<CommittedLibraryChange>, QString> bandcampDownload(
        QTcpSocket &tcpSocket,
        const DownloadRequest &downloadRequest,
        const QString &mediaDir,
        const QString &tempDir);

Result<QVector<CommittedLibraryChange>, QString> youtubeDownload(
        QTcpSocket &tcpSocket,
        const DownloadRequest &downloadRequest,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir);

Result<QVector<CommittedLibraryChange>, QString> download(
        QTcpSocket &tcpSocket,
        const DownloadRequest &downloadRequest,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir)
{
    switch (downloadRequest.requestType.data()) {
    case DownloadRequestType::BandcampAlbum:
        return bandcampDownload(tcpSocket, downloadRequest, mediaDir, tempDir);
    case DownloadRequestType::YoutubeVideo:
        return youtubeDownload(tcpSocket, downloadRequest, mediaDir, toolDir, tempDir);
    default:
        return {};
    }
}

static QByteArray download(QNetworkAccessManager &network, const QByteArray &url)
{
    QNetworkReply *reply = network.get(QNetworkRequest(QUrl(url)));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const QByteArray data = reply->readAll();
    delete reply;
    return data;
}

Result<QVector<CommittedLibraryChange>, QString> bandcampDownload(
        QTcpSocket &tcpSocket,
        const DownloadRequest &downloadRequest,
        const QString &mediaDir,
        const QString &tempDir)
{
#define REQUIRE(condition) do { if (!(condition)) { return QString(#condition); } } while (0)

    REQUIRE(downloadRequest.requestType == DownloadRequestType::BandcampAlbum);

    QByteArray out, err;

    // get album info
    QNetworkAccessManager network;
    const QByteArray albumHtml = download(network, downloadRequest.url->toUtf8());
    ScrapeBandcamp::ResultList albumInfo = ScrapeBandcamp::albumInfo(std::string(albumHtml.data()));

    // download tracks into mp3 files
    const QString dstDir = createTempDir(tempDir) + "/";
    QStringList mp3TempFiles;
    for (size_t i = 0; i < albumInfo.size(); ++i) {
        const QByteArray mp3 = download(network, QByteArray(albumInfo[i].mp3url.data()));
        QFile file(dstDir + QString::number(i) + ".mp3");
        REQUIRE(file.open(QIODevice::WriteOnly));
        REQUIRE(file.write(mp3) == mp3.size());
        mp3TempFiles << file.fileName();
    }

    ChangeSet changeSet(tcpSocket);

    // add artist, if not yet existing
    ArtistId artistId = *downloadRequest.artistId;
    if (!artistId.isValid()) {
        const LibraryChangeRequest addArtist(LibraryChangeRequest::ArtistAddOrGet, 0, 0, downloadRequest.artistName);
        auto result = changeSet.request({addArtist});
        if (result.hasError())
            return result.takeError();

        QVector<CommittedLibraryChange> resultChanges = result.takeValue();
        artistId = resultChanges.first().createdId;
    }

    // add album to database
    const LibraryChangeRequest addAlbum(LibraryChangeRequest::AlbumAdd, artistId, 0, downloadRequest.albumName);
    auto addAlbumResult = changeSet.request({addAlbum});
    if (addAlbumResult.hasError())
        return addAlbumResult.takeError();
    const AlbumId albumId = addAlbumResult->first().createdId;

    // add songs to database
    QVector<LibraryChangeRequest> songChanges;
    for (const ScrapeBandcamp::Result &song : albumInfo)
        songChanges << LibraryChangeRequest(LibraryChangeRequest::SongAdd, albumId, 0, QString::fromStdString(song.trackName));
    auto songChangesResult = changeSet.request(songChanges);
    if (songChangesResult.hasError())
        return songChangesResult.takeError();

    // move songs to media directory
    for (uint i = 0; i < albumInfo.size(); ++i) {
        const QString newFilePath = mediaDir + QString::asprintf("/%d.mp3", songChangesResult->at(i).createdId);
        QFile(mp3TempFiles[i]).rename(newFilePath);
    }

    // remove temp. directory
    QDir().rmdir(dstDir);

    // set library information for all new files
    QVector<LibraryChangeRequest> songDetails;
    for (uint i = 0; i < albumInfo.size(); ++i) {
        const quint32 id = songChangesResult->at(i).createdId;
        songDetails << LibraryChangeRequest(LibraryChangeRequest::SongSetFileEnding, id, 0, ".mp3");
        songDetails << LibraryChangeRequest(LibraryChangeRequest::SongSetLength, id, albumInfo[i].mp3duration, "");
        songDetails << LibraryChangeRequest(LibraryChangeRequest::SongSetPosition, id, i+1, "");
    }
    auto songDetailsResult = changeSet.request(songDetails);
    if (songDetailsResult.hasError())
        return songDetailsResult.takeError();

    return changeSet.changes();

#undef REQUIRE
}

Result<QVector<CommittedLibraryChange>, QString> youtubeDownload(
        QTcpSocket &tcpSocket,
        const DownloadRequest &downloadRequest,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir)
{
#define REQUIRE(condition) do { if (!(condition)) { return QString(#condition); } } while (0)

    REQUIRE(downloadRequest.requestType == DownloadRequestType::YoutubeVideo);

    QByteArray out, err;
    QVector<CommittedLibraryChange> ret;

    // download video into temp. file
    const QStringList args{"--quiet", "--ignore-errors", "--extract-audio", "--exec", "echo {}", downloadRequest.url};
    QByteArray dstFileName;
    int status = runProcess(toolDir + "/youtube-dl", args, &dstFileName, &err, 120000);
    if (status != 0) {
        qWarning() << "youtube-dl failed, status =" << status << "args =" << args.join(" ");
        qWarning() << "stdout:";
        qWarning().noquote() << out;
        qWarning() << "stderr:";
        qWarning().noquote() << err;
        return {};
    }
    dstFileName = dstFileName.split('\n').first().trimmed();
    REQUIRE(QFile::exists(dstFileName));
    const QString ending = dstFileName.split('.').last();

    struct ResultSong {
        QString title;
        QString file;
        int duration;
    };
    QVector<ResultSong> resultSongs;

    // get meta-info, and maybe split file into multiple chapters
    const YoutubeInfo videoInfo = parseInfo(downloadRequest.url, toolDir);
    bool hasChapters = !videoInfo.chapters.isEmpty();
    for (const YoutubeChapter &chapter : videoInfo.chapters) {
        const QString chapterFile = createTempFile(tempDir, QString(".") + ending);
        REQUIRE(QFile::exists(chapterFile));
        const int ffmpegStatus = runProcess(toolDir + "/ffmpeg", QStringList{
            "-ss", QString::number(chapter.startTime), "-t", QString::number(chapter.endTime),
            "-i", dstFileName, "-y", "-c", "copy", chapterFile}, &out, &err, 120000);

        // TODO: force overwrite for ffmpeg
        if (ffmpegStatus != 0) {
            qWarning() << "Couldn't split file into chapters";
            hasChapters = false;
            break;
        }
        resultSongs << ResultSong{chapter.title, chapterFile, chapter.endTime - chapter.startTime};
    }
    if (resultSongs.isEmpty())
        resultSongs << ResultSong{videoInfo.title, dstFileName, videoInfo.duration};

    ChangeSet changeSet(tcpSocket);

    // add artist, if not yet existing
    ArtistId artistId = *downloadRequest.artistId;
    if (!artistId.isValid()) {
        const LibraryChangeRequest addArtist(LibraryChangeRequest::ArtistAddOrGet, 0, 0, downloadRequest.artistName);
        auto result = changeSet.request({addArtist});
        if (result.hasError())
            return result.takeError();

        QVector<CommittedLibraryChange> resultChanges = result.takeValue();
        artistId = resultChanges.first().createdId;
    }

    // add album to database
    const LibraryChangeRequest addAlbum(LibraryChangeRequest::AlbumAdd, artistId, 0, downloadRequest.albumName);
    auto addAlbumResult = changeSet.request({addAlbum});
    if (addAlbumResult.hasError())
        return addAlbumResult.takeError();
    const AlbumId albumId = addAlbumResult->first().createdId;

    // add songs to database
    QVector<LibraryChangeRequest> songChanges;
    for (const ResultSong &song : resultSongs)
        songChanges << LibraryChangeRequest(LibraryChangeRequest::SongAdd, albumId, 0, song.title);
    auto songChangesResult = changeSet.request(songChanges);
    if (songChangesResult.hasError())
        return songChangesResult.takeError();

    // move songs to media directory
    const QVector<CommittedLibraryChange> committedSongChanges = songChangesResult.takeValue();
    for (int i = 0; i < committedSongChanges.size(); ++i) {
        const quint32 songId = committedSongChanges[i].createdId;
        const QString newFilePath = mediaDir + "/" + QString::number(songId) + "." + ending;
        QFile(resultSongs[i].file).rename(newFilePath);
    }

    // remove temp. file, if it still exists
    if (hasChapters)
        QFile(dstFileName).remove();

    // set library information for all new files
    QVector<LibraryChangeRequest> songDetails;
    for (int i = 0; i < committedSongChanges.size(); ++i) {
        const quint32 id = committedSongChanges[i].createdId;
        songDetails << LibraryChangeRequest(LibraryChangeRequest::SongSetFileEnding, id, 0, QString(".") + ending);
        songDetails << LibraryChangeRequest(LibraryChangeRequest::SongSetLength, id, resultSongs[i].duration, "");
        songDetails << LibraryChangeRequest(LibraryChangeRequest::SongSetPosition, id, i + 1    , "");
    }
    auto songDetailsResult = changeSet.request(songDetails);
    if (songDetailsResult.hasError())
        return songDetailsResult.takeError();

    return changeSet.changes();

#undef REQUIRE
}
