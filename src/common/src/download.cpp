#include "download.hpp"
#include "util.hpp"
#include "jsonconv.hpp"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static QString createTempDir(const QString &dir)
{
    QProcess::execute("mkdir -p " + dir);
    QByteArray out;
    ClientCommon::runProcess("mktemp", {"-d", "-p", dir}, &out, nullptr, 1000);
    return QString(out).trimmed();
}

static QString createTempFile(const QString &dir, const QString &suffix = QString())
{
    QProcess::execute("mkdir -p " + dir);
    QByteArray out;
    ClientCommon::runProcess("tempfile", {"-d", dir, "-s", suffix}, &out, nullptr, 1000);
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
    QByteArray out, err;
    int status = ClientCommon::runProcess(toolDir + "/node", {toolDir + "/get-youtube-info.js", url}, &out, &err, 60000);

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

namespace ClientCommon {

QVector<Moosick::CommittedLibraryChange> bandcampDownload(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir);

QVector<Moosick::CommittedLibraryChange> youtubeDownload(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir);

QVector<Moosick::CommittedLibraryChange> download(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir)
{
    switch (request.tp) {
    case NetCommon::DownloadRequest::BandcampAlbum:
        return bandcampDownload(server, request, mediaDir, toolDir, tempDir);
    case NetCommon::DownloadRequest::YoutubeVideo:
        return youtubeDownload(server, request, mediaDir, toolDir, tempDir);
    default:
        return {};
    }
}

QVector<Moosick::CommittedLibraryChange> bandcampDownload(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir)
{
    QVector<Moosick::CommittedLibraryChange> ret;

#define REQUIRE(condition) do { if (!(condition)) { qWarning() << "Failed:" << #condition; return {}; } } while (0)

    REQUIRE(request.tp == NetCommon::DownloadRequest::BandcampAlbum);

    // get info about downloads
    QByteArray out, err;
    int status = runProcess(toolDir + "/node", {toolDir + "/bandcamp-album-info.js", request.url}, &out, &err, 20000);
    if (status != 0) {
        qWarning() << "Node.js failed:";
        qWarning() << "stdout:";
        qWarning().noquote() << out;
        qWarning() << "stderr:";
        qWarning().noquote() << err;
        return {};
    }

    QJsonParseError jsonError;
    const QJsonDocument albumInfoJsonDoc = QJsonDocument::fromJson(out, &jsonError);
    if (albumInfoJsonDoc.isNull()) {
        qWarning() << "Failed to parse JSON:";
        qWarning().noquote() << out;
        qWarning() << "Error:";
        qWarning().noquote() << jsonError.errorString();
        return {};
    }

    NetCommon::BandcampAlbumInfo albumInfo;
    REQUIRE(albumInfo.fromJson(albumInfoJsonDoc.object()));

    const int numSongs = albumInfo.tracks.size();

    // download album into temp. directory
    const QString dstDir = createTempDir(tempDir) + "/";
    REQUIRE(!dstDir.isEmpty());
    status = runProcess(toolDir + "/bandcamp-dl", {
                   QString("--base-dir=") + dstDir,
                   "--overwrite",
                   "--template=%{track}",
                   request.url
               }, &out, &err, 120000, "yes");

    if (status != 0) {
        qWarning() << "bandcamp-dl failed, status =" << status;
        qWarning() << "stdout:";
        qWarning().noquote() << out;
        qWarning() << "stderr:";
        qWarning().noquote() << err;
        return {};
    }

    // check if the files are there
    QStringList tempFilePaths;
    for (int i = 1; i <= numSongs; ++i) {
        const QString tempFilePath = dstDir + QString::asprintf("/%02d.mp3", i);
        tempFilePaths << tempFilePath;
        REQUIRE(QFile::exists(tempFilePath));
    }

    // add artist, if not yet existing
    Moosick::ArtistId artistId = request.artistId;
    QVector<Moosick::CommittedLibraryChange> resultChanges;
    if (!artistId.isValid()) {
        const Moosick::LibraryChange addArtist(Moosick::LibraryChange::ArtistAddOrGet, 0, 0, request.artistName);
        REQUIRE(sendChanges(server, {addArtist}, resultChanges));
        REQUIRE(!resultChanges.isEmpty());
        REQUIRE(resultChanges.first().change.changeType == Moosick::LibraryChange::ArtistAddOrGet);
        REQUIRE(resultChanges.first().change.detail != 0);
        artistId = resultChanges.first().change.detail;
        ret << resultChanges;
    }

    // add album to database
    const Moosick::LibraryChange addAlbum(Moosick::LibraryChange::AlbumAdd, artistId, 0, request.albumName);
    REQUIRE(sendChanges(server, {addAlbum}, resultChanges));
    REQUIRE(!resultChanges.isEmpty());
    REQUIRE(resultChanges.first().change.changeType == Moosick::LibraryChange::AlbumAdd);
    REQUIRE(resultChanges.first().change.detail != 0);
    ret << resultChanges;
    const Moosick::AlbumId albumId = resultChanges.first().change.detail;

    // add songs to database
    QVector<Moosick::LibraryChange> songChanges;
    for (const NetCommon::BandcampSongInfo &song : albumInfo.tracks)
        songChanges << Moosick::LibraryChange(Moosick::LibraryChange::SongAdd, albumId, 0, song.name);
    REQUIRE(sendChanges(server, songChanges, resultChanges));
    REQUIRE(resultChanges.size() == songChanges.size());
    for (const Moosick::CommittedLibraryChange &result : resultChanges) {
        REQUIRE(result.change.changeType == Moosick::LibraryChange::SongAdd);
        REQUIRE(result.change.detail != 0);
    }
    ret << resultChanges;

    // move songs to media directory
    for (int i = 0; i < numSongs; ++i) {
        const QString newFilePath = mediaDir + QString::asprintf("/%d.mp3", resultChanges[i].change.detail);
        QFile(tempFilePaths[i]).rename(newFilePath);
    }

    // remove temp. directory
    QProcess::execute(toolDir + "/rm", {"-rf", dstDir});

    // set library information for all new files
    QVector<Moosick::LibraryChange> songDetails;
    for (int i = 0; i < numSongs; ++i) {
        const quint32 id = resultChanges[i].change.detail;
        songDetails << Moosick::LibraryChange(Moosick::LibraryChange::SongSetFileEnding, id, 0, ".mp3");
        songDetails << Moosick::LibraryChange(Moosick::LibraryChange::SongSetLength, id, albumInfo.tracks[i].secs, "");
        songDetails << Moosick::LibraryChange(Moosick::LibraryChange::SongSetPosition, id, i+1, "");
    }
    REQUIRE(sendChanges(server, songDetails, resultChanges));
    REQUIRE(resultChanges.size() == songDetails.size());
    ret << resultChanges;

    // get whole change-list from beginning from server
    const ClientCommon::Message changeListRequest{ ClientCommon::ChangeListRequest, QByteArray::number(request.currentRevision) };
    ClientCommon::Message answer;
    REQUIRE(sendRecv(server, changeListRequest, answer));

    QVector<Moosick::CommittedLibraryChange> appliedChanges;
    REQUIRE(fromJson(parseJsonArray(answer.data, ""), appliedChanges));

    return appliedChanges;

#undef REQUIRE
}

QVector<Moosick::CommittedLibraryChange> youtubeDownload(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir)
{
    QByteArray out, err;
    QVector<Moosick::CommittedLibraryChange> ret;

#define REQUIRE(condition) do { if (!(condition)) { qWarning() << "Failed:" << #condition; return {}; } } while (0)

    REQUIRE(request.tp == NetCommon::DownloadRequest::YoutubeVideo);

    // download video into temp. file
    const QStringList args{"--quiet", "--ignore-errors", "--extract-audio", "--exec", "echo {}", request.url};
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
    const YoutubeInfo videoInfo = parseInfo(request.url, toolDir);
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

    // add artist, if not yet existing
    Moosick::ArtistId artistId = request.artistId;
    QVector<Moosick::CommittedLibraryChange> resultChanges;
    if (!artistId.isValid()) {
        const Moosick::LibraryChange addArtist(Moosick::LibraryChange::ArtistAddOrGet, 0, 0, request.artistName);
        REQUIRE(sendChanges(server, {addArtist}, resultChanges));
        REQUIRE(!resultChanges.isEmpty());
        REQUIRE(resultChanges.first().change.changeType == Moosick::LibraryChange::ArtistAddOrGet);
        REQUIRE(resultChanges.first().change.detail != 0);
        artistId = resultChanges.first().change.detail;
        ret << resultChanges;
    }

    // add album to database
    const Moosick::LibraryChange addAlbum(Moosick::LibraryChange::AlbumAdd, artistId, 0, request.albumName);
    REQUIRE(sendChanges(server, {addAlbum}, resultChanges));
    REQUIRE(!resultChanges.isEmpty());
    REQUIRE(resultChanges.first().change.changeType == Moosick::LibraryChange::AlbumAdd);
    REQUIRE(resultChanges.first().change.detail != 0);
    ret << resultChanges;
    const Moosick::AlbumId albumId = resultChanges.first().change.detail;

    // add song(s) to database
    QVector<Moosick::LibraryChange> songChanges;
    for (const ResultSong &song : resultSongs)
        songChanges << Moosick::LibraryChange(Moosick::LibraryChange::SongAdd, albumId, 0, song.title);
    REQUIRE(sendChanges(server, songChanges, resultChanges));
    REQUIRE(resultChanges.size() == songChanges.size());
    for (const Moosick::CommittedLibraryChange &result : resultChanges) {
        REQUIRE(result.change.changeType == Moosick::LibraryChange::SongAdd);
        REQUIRE(result.change.detail != 0);
    }
    ret << resultChanges;

    // move songs to media directory
    REQUIRE(!ending.isEmpty());
    for (int i = 0; i < resultChanges.size(); ++i) {
        const quint32 songId = resultChanges[i].change.detail;
        const QString newFilePath = mediaDir + "/" + QString::number(songId) + "." + ending;
        QFile(resultSongs[i].file).rename(newFilePath);
    }

    // set library information for new song
    QVector<Moosick::LibraryChange> songDetails;
    for (int i = 0; i < resultChanges.size(); ++i) {
        const quint32 songId = resultChanges[i].change.detail;
        songDetails << Moosick::LibraryChange(Moosick::LibraryChange::SongSetFileEnding, songId, 0, QString(".") + ending);
        songDetails << Moosick::LibraryChange(Moosick::LibraryChange::SongSetLength, songId, resultSongs[i].duration, "");
        songDetails << Moosick::LibraryChange(Moosick::LibraryChange::SongSetPosition, songId, i + 1    , "");
    }
    REQUIRE(sendChanges(server, songDetails, resultChanges));
    REQUIRE(resultChanges.size() == songDetails.size());
    ret << resultChanges;

    // get whole change-list from beginning from server
    const ClientCommon::Message changeListRequest{ ClientCommon::ChangeListRequest, QByteArray::number(request.currentRevision) };
    ClientCommon::Message answer;
    REQUIRE(sendRecv(server, changeListRequest, answer));

    QVector<Moosick::CommittedLibraryChange> appliedChanges;
    REQUIRE(fromJson(parseJsonArray(answer.data, ""), appliedChanges));

    if (hasChapters)
        QFile(dstFileName).remove();

    return appliedChanges;

#undef REQUIRE
}

} //namespace ClientCommon
