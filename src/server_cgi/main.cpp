#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <QDebug>
#include <QTcpSocket>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <iostream>

#include "library.hpp"
#include "library_messages.hpp"
#include "jsonconv.hpp"
#include "serversettings.hpp"
#include "tcpclientserver.hpp"
#include "logger.hpp"

using namespace MoosickMessage;

static bool execute(const QString &program, const QStringList &args, QByteArray &out)
{
    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(args);
    proc.start();
    if (proc.waitForFinished(3600000) && proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
        out = proc.readAllStandardOutput();
        return true;
    }
    else {
        qWarning() << program << args << "failed:" << proc.error() << proc.exitCode() << proc.exitStatus();
        qWarning() << "stdout:";
        qWarning().noquote() << proc.readAllStandardOutput();
        qWarning() << "stderr:";
        qWarning().noquote() << proc.readAllStandardError();
        return false;
    }
}

static Message getYoutubeUrl(const QString &toolDir, const QString &videoId)
{
    const QString url = QString("https://www.youtube.com/watch?v=") + videoId;
    QByteArray youtubeDlOut;
    if (!execute(toolDir + "/youtube-dl", {"-j", url}, youtubeDlOut))
        return new Error("Internal Error");

    const QJsonDocument jsonDoc = QJsonDocument::fromJson(youtubeDlOut);
    if (!jsonDoc.isObject())
        return new Error("Internal Error");
    const QJsonObject json = jsonDoc.object();

    const auto formatsIt = json.find("formats");
    if (formatsIt == json.end() || !formatsIt->isArray())
        return new Error("Internal Error");
    const QJsonArray formats = formatsIt->toArray();

    // find best audio URL among formats: no DASH, 'audio only', and largest file size
    int bestAudioSize = 0;
    QString bestUrl;
    for (const QJsonValue &format : formats) {
        const QString formatName = format["format"].toString();

        // ain't nobody got love for DASH
        if (formatName.toLower().contains("dash"))
            continue;

        const int size = format["filesize"].toInt();
        bool isBest = bestUrl.isEmpty();
        if (!isBest && formatName.contains("audio only") && size && size > bestAudioSize)
            isBest = true;

        if (isBest) {
            bestUrl = format["url"].toString();
            bestAudioSize = size;
        }
    }

    if (bestUrl.isEmpty())
        return new Error("Internal Error");

    YoutubeUrlResponse *response = new YoutubeUrlResponse();
    response->title = json["title"].toString();
    response->duration = json["duration"].toInt();
    response->chapters = json["chapters"].toArray();
    response->url = bestUrl;
    return response;
}

static Message sendMessage(const QString &host, quint16 port, const Message &message)
{
    QTcpSocket tcpSocket;
    tcpSocket.connectToHost(host, port);
    if (!tcpSocket.waitForConnected(1000)) {
        qWarning().noquote() << "Can't connect to" << host << port;
        return new Error("Internal error");
    }

    const QByteArray messageJson = message.toJson();
    auto result = TcpClient::sendMessage(tcpSocket, messageJson, 10000);
    if (!result.hasValue()) {
        qWarning().noquote() << "Failed to send/recv TCP message:" << result.takeError();
        return new Error("Internal error");
    }

    const QByteArray resultData = result.takeValue();
    Result<Message, EnjsonError> resultMessage = Message::fromJson(resultData);
    if (!resultMessage.hasValue()) {
        qWarning().noquote() << "Failed to parse JSON response from" << host;
        qWarning().noquote() << "Error:" << resultMessage.takeError().toString();
        qWarning().noquote() << "Sent message:";
        qWarning().noquote() << messageJson;
        qWarning().noquote() << "Received message:";
        qWarning().noquote() << resultData;
        return new Error("Internal error");
    }

    return resultMessage.takeValue();
}

static Message uploadFile(const ServerSettings &settings, const Message &message, const QByteArray &postData)
{
    const QString tmpDir = settings.tempDir();

    const UploadSongRequest *uploadRequest = message.as<UploadSongRequest>();
    if (uploadRequest->fileSize != postData.size())
        return new Error("Specified file size doesn't match POST data");

    // write data to temp file
    QDir().mkpath(tmpDir);
    const quint64 rand64 = QRandomGenerator::global()->generate64();
    const QString tempFileName = tmpDir + QDir::separator() + QString::number(rand64);
    QFile tempFile(tempFileName);
    if (!tempFile.open(QIODevice::WriteOnly))
        return new Error("Internal error");

    tempFile.write(postData);

    UploadSongRequestInternal *internalRequest = new UploadSongRequestInternal();
    internalRequest->artistName = uploadRequest->artistName;
    internalRequest->albumName = uploadRequest->albumName;
    internalRequest->title = uploadRequest->title;
    internalRequest->position = uploadRequest->position;
    internalRequest->duration = uploadRequest->duration;
    internalRequest->fileEnding = uploadRequest->fileEnding;
    internalRequest->filePath = tempFileName;

    return sendMessage(settings.dbserverHost(), settings.dbserverPort(), internalRequest);
}

/**
 * if the message was passed via &message64= query, the POST data
 * is available separately.
 * Otherwise, postData will just contain the message that has already been parsed.
 */
static Message handleMessage(const ServerSettings &settings, const Message &message, const QByteArray &postData)
{
    const QString toolDir = settings.toolsDir();

    switch (message.getType()) {
    case Type::Ping: {
        return new Pong();
    }
    // in this case, we expect the file data to be POSTed
    case Type::UploadSongRequest: {
        return uploadFile(settings, message, postData);
    }
    // forward messages to DB server
    case Type::LibraryRequest:
    case Type::IdRequest:
    case Type::ChangesRequest:
    case Type::ChangeListRequest:
    case Type::DownloadRequest:
    case Type::DownloadQuery: {
        return sendMessage(settings.dbserverHost(), settings.dbserverPort(), message);
    }
    case Type::YoutubeUrlQuery: {
        const YoutubeUrlQuery *query = message.as<YoutubeUrlQuery>();
        return getYoutubeUrl(toolDir, query->videoId);
    }
    case Type::MediaUrlRequest: {
        return new MediaUrlResponse(settings.mediaBaseUrl());
    }
    default: {
        return new Error("Unhandled message type");
    }
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

#ifdef Q_OS_WIN32
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    const ServerSettings settings;
    if (!settings.isValid()) {
        qWarning() << "Settings file not valid";
        return 1;
    }

    if (!settings.cgiLogFile().isEmpty()) {
        Logger::setLogFile(settings.cgiLogFile());
        Logger::setLogFileFilter(settings.cgiLogLevel());
        Logger::install();
    }

    // For file uploads we don't want to go through JSON, as
    // this would incur a huge overhead for parsing and base64 decoding.
    // Instead, we pass the request message base64-encoded via query string,
    // and the actual file data raw via POST
    QByteArray queryMessage;
    const QByteArray requestUri = qgetenv("REQUEST_URI");
    static const char* message64Query = "?message64=";
    const int message64Index = requestUri.indexOf(message64Query);
    if (message64Index >= 0) {
        const QByteArray message64 = requestUri.mid(message64Index + strlen(message64Query));
        queryMessage = QByteArray::fromBase64(message64, QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    }

    // Read POST data from stdin
    QByteArray postData;
    const int contentLength = qgetenv("CONTENT_LENGTH").toInt();
    if (contentLength > 0) {
        while(postData.size() < contentLength && !std::cin.eof()) {
            constexpr int bufferSize = 1024;
            char buffer[bufferSize];
            std::cin.read(buffer, qMin(bufferSize, contentLength - postData.size()));
            const int read = std::cin.gcount();
            postData.append(buffer, read);
        }
    }

    const QByteArray messageJson = queryMessage.isEmpty() ? postData : queryMessage;
    Result<Message, EnjsonError> messageParsingResult = Message::fromJson(messageJson);

    if (messageParsingResult.hasError()) {
        Error error;
        error.errorMessage = "Failed to parse message";
        qWarning().noquote() << "Failed to parse message:" << messageParsingResult.getError().toString();
        if (messageJson.size() < 1024)
            qWarning().noquote() << messageJson;
        else
            qWarning() << "Omitting message, size =" << messageJson.size();
        std::cout << Message(error).toJson().constData() << std::endl;
        return 0;
    }

    Message message = messageParsingResult.takeValue();
    Message response = handleMessage(settings, message, postData);

    std::cout << response.toJson().constData() << std::endl;

    return 0;
}
