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

static const char *LIB_SERVER_PORT = "LIB_SERVER_PORT";
static const char *DOWNLOAD_SERVER_PORT = "DOWNLOAD_SERVER_PORT";

static QByteArray getCommand(QByteArray request)
{
    while (!request.isEmpty() && request[0] == '/')
        request.remove(0, 1);
    return request;
}

int main(int argc, char **argv)
{
    const QByteArray request = qgetenv("REQUEST_URI");
    if (request.isEmpty() || !request.startsWith("/"))
        return 0;

    // extract command and key/value pairs
    const QList<QByteArray> parts = request.mid(1).split('?');
    const QByteArray command = getCommand(parts[0]);
    QHash<QByteArray, QByteArray> values;
    for (const QByteArray &keyValue : parts.value(1).split('&')) {
        const QList<QByteArray> kvPair = keyValue.split('=');
        if (kvPair.size() == 2)
            values[kvPair[0]] = kvPair[1];
    }

    const QString rootDir = qgetenv("DOCUMENT_ROOT");
    const QString toolDir = rootDir + "/../tools/";
    const QString jsDir = toolDir + "scrapers/";
    const QString tmpDir = "/tmp/";
    const QString mediaDir = rootDir;

    ClientCommon::ServerConfig libraryServer{ "localhost", 12345, 1000 };
    ClientCommon::ServerConfig downloadServer{ "localhost", 54321, 1000 };

    if (qEnvironmentVariableIsEmpty(LIB_SERVER_PORT))
        libraryServer.port = qEnvironmentVariable(LIB_SERVER_PORT).toUShort();
    if (qEnvironmentVariableIsEmpty(DOWNLOAD_SERVER_PORT))
        downloadServer.port = qEnvironmentVariable(DOWNLOAD_SERVER_PORT).toUShort();

    if (command == "lib.do") {
        ClientCommon::Message answer;
        sendRecv(libraryServer, ClientCommon::Message{ ClientCommon::LibraryRequest }, answer);

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
            QProcess::execute(toolDir + "node", {jsDir + "search.js", searchPattern});
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
            QProcess::execute(toolDir + "node", {jsDir + "bandcamp-artist-info.js", url});
        } else
            std::cout << "[]" << std::endl;
        return 0;
    }
    else if (command == "bandcamp-album-info.do") {
        if (values.contains("v") && !values["v"].isEmpty()) {
            const QString url = values["v"];
            QProcess::execute(toolDir + "node", {jsDir + "bandcamp-album-info.js", url});
        } else
            std::cout << "[]" << std::endl;
        return 0;
    }

    return 0;
}
