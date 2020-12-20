#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <iostream>

#include "library.hpp"
#include "messages.hpp"
#include "requests.hpp"
#include "jsonconv.hpp"
#include "serversettings.hpp"

static QByteArray getCommand(QByteArray request)
{
    while (!request.isEmpty() && request[0] == '/')
        request.remove(0, 1);
    return request;
}

bool execute(const QString &program, const QStringList &args, QByteArray &out)
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

bool printYoutubeUrl(const QString &toolDir, const QString &videoId)
{
    const QString url = QString("https://www.youtube.com/watch?v=") + videoId;
    QByteArray youtubeDlOut;
    if (!execute(toolDir + "/youtube-dl", {"-j", url}, youtubeDlOut))
        return false;

    const QJsonDocument jsonDoc = QJsonDocument::fromJson(youtubeDlOut);
    if (!jsonDoc.isObject())
        return false;
    const QJsonObject json = jsonDoc.object();

    const auto formatsIt = json.find("formats");
    if (formatsIt == json.end() || !formatsIt->isArray())
        return false;
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
        return false;

    QJsonObject out;
    out["title"] = json["title"].toString();
    out["duration"] = json["duration"].toInt();
    out["chapters"] = json["chapters"].toArray();
    out["url"] = bestUrl;

    std::cout << QJsonDocument(out).toJson().data() << std::endl;
    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const ServerSettings settings;
    if (!settings.isValid()) {
        qWarning() << "Settings file not valid";
        return 1;
    }

    QByteArray contentBytes;
    const int contentLength = qgetenv("CONTENT_LENGTH").toInt();
    if (contentLength > 0) {
        while(!std::cin.eof()) {
            char arr[1024];
            std::cin.read(arr,sizeof(arr));
            int s = std::cin.gcount();
            contentBytes.append(arr,s);
        }
    }

    const QByteArray request = qgetenv("REQUEST_URI");
    if (request.isEmpty() || !request.startsWith("/music/")) {
        qWarning() << "REQUEST_URI invalid:" << request;
        return 0;
    }

    // extract command
    const QList<QByteArray> parts = request.mid(1).split('?');
    const QByteArray command = getCommand(parts[0]);

    // extract key-value pairs, if this is not a POST
    const QByteArray keyValueString = contentBytes.isEmpty() ? parts.value(1) : contentBytes;
    QHash<QByteArray, QByteArray> values;
    for (const QByteArray &keyValue : keyValueString.split('&')) {
        const QList<QByteArray> kvPair = keyValue.split('=');
        if (kvPair.size() == 2)
            values[kvPair[0]] = kvPair[1];
    }

    const QString rootDir = settings.serverRoot();
    const QString toolDir = settings.toolsDir();
    const QString jsDir = toolDir + "scrapers/";
    const QString tmpDir = settings.tempDir();
    const QString mediaDir = settings.serverRoot();

    ClientCommon::ServerConfig libraryServer{ "localhost", settings.dbserverPort(), 1000 };
    ClientCommon::ServerConfig downloadServer{ "localhost", settings.downloaderPort(), 1000 };

    if (command == "lib.do") {
        ClientCommon::Message answer;
        sendRecv(libraryServer, ClientCommon::Message{ ClientCommon::LibraryRequest }, answer);

        std::cout << answer.data.constData() << "\n";

        return 0;
    }
    else if (command == "id.do") {
        ClientCommon::Message answer;
        sendRecv(libraryServer, ClientCommon::Message{ ClientCommon::IdRequest }, answer);

        std::cout << answer.data.constData() << "\n";

        return 0;
    }
    else if (command == "get-change-list.do") {
        if (values.contains("v") && !values["v"].isEmpty()) {
            const ClientCommon::Message request{ ClientCommon::ChangeListRequest, values["v"] };
            ClientCommon::Message answer;
            sendRecv(libraryServer, request, answer);

            std::cout << answer.data.constData() << "\n";
        } else {
            std::cout << "[]" << std::endl;
        }

        return 0;
    }
    else if (command == "request-changes.do") {
        if (values.contains("v") && !values["v"].isEmpty()) {
            QVector<Moosick::LibraryChange> changes;

            // parse changes
            QDataStream reader(QByteArray::fromBase64(values["v"], QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
            reader >> changes;

            ClientCommon::Message request{ ClientCommon::ChangesRequest };
            QDataStream writer(&request.data, QIODevice::WriteOnly);
            writer << changes;

            ClientCommon::Message answer;
            sendRecv(libraryServer, request, answer);

            std::cout << answer.data.constData() << "\n";
        } else {
            std::cout << "[]" << std::endl;
        }

        return 0;
    }
    else if (command == "get-youtube-url.do") {
        if (!values.contains("v") || values["v"].isEmpty())
            return 0;

        if (!printYoutubeUrl(toolDir, values["v"]))
            std::cout << "{}" << std::endl;
        return 0;
    }
    else if (command == "download.do") {
        if (!values.contains("v") || values["v"].isEmpty())
            return 0;

        ClientCommon::Message answer, request{ ClientCommon::DownloadRequest, values["v"] };
        sendRecv(downloadServer, request, answer);
        std::cout << answer.data.constData() << "\n";
        return 0;
    }
    else if (command == "running-downloads.do") {
        ClientCommon::Message answer, request{ ClientCommon::DownloadQuery, "" };
        sendRecv(downloadServer, request, answer);
        std::cout << answer.data.constData() << "\n";
        return 0;
    }

    return 0;
}
