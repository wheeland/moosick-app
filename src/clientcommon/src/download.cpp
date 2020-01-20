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
    QByteArray out;
    ClientCommon::runProcess("mktemp", {"-d", "-p", dir}, &out, nullptr, 1000);
    return QString(out).trimmed();
}

namespace ClientCommon {

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
    QProcess::execute("mkdir -p " + tempDir);
    const QString dstDir = createTempDir(tempDir) + "/";
    status = runProcess(toolDir + "/bandcamp-dl", {
                   QString("--base-dir=") + dstDir,
                   "--template=%{track}",
                   request.url
               }, &out, &err, 120000);

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
        const Moosick::LibraryChange addArtist(Moosick::LibraryChange::ArtistAdd, 0, 0, request.artistName);
        REQUIRE(sendChanges(server, {addArtist}, resultChanges));
        REQUIRE(!resultChanges.isEmpty());
        REQUIRE(resultChanges.first().change.changeType == Moosick::LibraryChange::ArtistAdd);
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

} //namespace ClientCommon
