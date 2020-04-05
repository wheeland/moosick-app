#include <QCoreApplication>
#include <QProcess>
#include <QJsonDocument>
#include <iostream>

#include "library.hpp"
#include "messages.hpp"
#include "requests.hpp"
#include "download.hpp"
#include "json.hpp"
#include "jsonconv.hpp"
#include "serversettings.hpp"

static QByteArray getCommand(QByteArray request)
{
    while (!request.isEmpty() && request[0] == '/')
        request.remove(0, 1);
    return request;
}

void execute(const QString &program, const QStringList &args)
{
    QProcess proc;
    proc.setProgram(program);
    proc.setArguments(args);
    proc.start();
    if (!proc.waitForFinished(3600000)) {
        qWarning() << program << args << "failed:" << proc.error() << proc.exitCode() << proc.exitStatus();
        qWarning() << "stdout:";
        qWarning().noquote() << proc.readAllStandardOutput();
        qWarning() << "stderr:";
        qWarning().noquote() << proc.readAllStandardError();
    }
    else {
        const QByteArray out = proc.readAllStandardOutput();
        std::cout << out.constData() << "\n";
    }
}

int main(int argc, char **argv)
{
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
    if (request.isEmpty() || !request.startsWith("/"))
        return 0;

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
    else if (command == "search.do") {
        if (values.contains("v") && !values["v"].isEmpty()) {
            const QString searchPattern = QByteArray::fromBase64(values["v"], QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
            execute(toolDir + "node", {jsDir + "search.js", searchPattern});
        } else
            std::cout << "[]" << std::endl;
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
    else if (command == "bandcamp-artist-info.do") {
        if (values.contains("v") && !values["v"].isEmpty()) {
            const QString url = values["v"];
            execute(toolDir + "node", {jsDir + "bandcamp-artist-info.js", url});
        } else
            std::cout << "[]" << std::endl;
        return 0;
    }
    else if (command == "bandcamp-album-info.do") {
        if (values.contains("v") && !values["v"].isEmpty()) {
            const QString url = values["v"];
            execute(toolDir + "node", {jsDir + "bandcamp-album-info.js", url});
        } else
            std::cout << "[]" << std::endl;
        return 0;
    }

    return 0;
}
