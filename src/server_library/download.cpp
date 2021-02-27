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

Result<DownloadResult, QString> bandcampDownload(
        const MoosickMessage::DownloadRequest &downloadRequest,
        const QString &tempDir);

Result<DownloadResult, QString> youtubeDownload(
        const MoosickMessage::DownloadRequest &downloadRequest,
        const QString &toolDir,
        const QString &tempDir);

Result<DownloadResult, QString> download(
        const MoosickMessage::DownloadRequest &downloadRequest,
        const QString &toolDir,
        const QString &tempDir)
{
    switch (downloadRequest.requestType.data()) {
    case DownloadRequestType::BandcampAlbum:
        return bandcampDownload(downloadRequest, tempDir);
    case DownloadRequestType::YoutubeVideo:
        return youtubeDownload(downloadRequest, toolDir, tempDir);
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

Result<DownloadResult, QString> bandcampDownload(
        const DownloadRequest &downloadRequest,
        const QString &tempDir)
{
#define REQUIRE(condition) do { if (!(condition)) { return QString(#condition); } } while (0)

    REQUIRE(downloadRequest.requestType == DownloadRequestType::BandcampAlbum);

    QByteArray out, err;

    // get album info
    QNetworkAccessManager network;
    const QByteArray albumHtml = download(network, downloadRequest.url->toUtf8());
    ScrapeBandcamp::ResultList albumInfo = ScrapeBandcamp::albumInfo(std::string(albumHtml.data()));

    QVector<DownloadResult::File> files;

    // download tracks into mp3 files
    const QString dstDir = createTempDir(tempDir) + "/";
    for (size_t i = 0; i < albumInfo.size(); ++i) {
        const QByteArray mp3 = download(network, QByteArray(albumInfo[i].mp3url.data()));
        QFile file(dstDir + QString::number(i) + ".mp3");
        REQUIRE(file.open(QIODevice::WriteOnly));
        REQUIRE(file.write(mp3) == mp3.size());
        DownloadResult::File outFile;
        outFile.title = QString::fromStdString(albumInfo[i].trackName);
        outFile.duration = albumInfo[i].mp3duration;
        outFile.fullPath = QFileInfo(file).absoluteFilePath();
        outFile.fileEnding = "mp3";
        outFile.albumPosition = albumInfo[i].trackNum;
        files << outFile;
    }

    DownloadResult ret;
    ret.artistId = downloadRequest.artistId;
    ret.artistName = downloadRequest.artistName;
    ret.albumName = downloadRequest.albumName;
    ret.files = files;
    ret.tempDir = QFileInfo(dstDir).absoluteFilePath();

    return ret;

#undef REQUIRE
}

Result<DownloadResult, QString> youtubeDownload(
        const DownloadRequest &downloadRequest,
        const QString &toolDir,
        const QString &tempDir)
{
#define REQUIRE(condition) do { if (!(condition)) { return QString(#condition); } } while (0)

    REQUIRE(downloadRequest.requestType == DownloadRequestType::YoutubeVideo);

    QByteArray out, err;

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

    QVector<DownloadResult::File> files;

    // get meta-info, and maybe split file into multiple chapters
    const YoutubeInfo videoInfo = parseInfo(downloadRequest.url, toolDir);
    bool hasChapters = !videoInfo.chapters.isEmpty();
    for (int i = 0; i < videoInfo.chapters.size(); ++i) {
        const YoutubeChapter &chapter = videoInfo.chapters[i];
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

        DownloadResult::File outFile;
        outFile.title = chapter.title;
        outFile.duration = chapter.endTime - chapter.startTime;
        outFile.fullPath = QFileInfo(chapterFile).absoluteFilePath();
        outFile.fileEnding = ending;
        outFile.albumPosition = i;
        files << outFile;
    }

    // no chapters? -> just return 1 single file
    if (!hasChapters) {
        DownloadResult::File outFile;
        outFile.title = videoInfo.title;
        outFile.duration = videoInfo.duration;
        outFile.fullPath = QFileInfo(dstFileName).absoluteFilePath();
        outFile.fileEnding = ending;
        outFile.albumPosition = 0;
        files.clear();
        files << outFile;
    }
    else {
        QFile::remove(dstFileName);
    }

    DownloadResult ret;
    ret.artistId = downloadRequest.artistId;
    ret.artistName = downloadRequest.artistName;
    ret.albumName = downloadRequest.albumName;
    ret.files = files;

    return ret;

#undef REQUIRE
}
