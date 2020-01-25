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

static ClientCommon::ServerConfig s_server;

template <class T>
QByteArray toBase64(const T& value)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << value;
    return data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

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

    s_server.hostName = "localhost";
    s_server.port = 12345;
    s_server.timeout = 1000;

    if (command == "lib.do") {
        ClientCommon::Message answer;
        sendRecv(s_server, ClientCommon::Message{ ClientCommon::LibraryRequest }, answer);

        std::cout << answer.data.constData() << "\n";

        return 0;
    }
    else if (command == "get-change-list.do") {
        if (values.contains("v") && !values["v"].isEmpty()) {
            const ClientCommon::Message request{ ClientCommon::ChangeListRequest, values["v"] };
            ClientCommon::Message answer;
            sendRecv(s_server, request, answer);

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
            sendRecv(s_server, request, answer);

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
    else if (command == "bandcamp-download.do") {
        if (!values.contains("v") || values["v"].isEmpty())
            return 0;

        NetCommon::DownloadRequest request;
        if (!request.fromBase64(values["v"])) {
            qWarning() << "Failed parsing the base64 download request";
            return 0;
        }

        const QVector<Moosick::CommittedLibraryChange> changes = ClientCommon::bandcampDownload(
                    s_server, request, mediaDir, toolDir, tmpDir);

        std::cout << QJsonDocument(toJson(changes).toArray()).toJson().constData() << std::endl;
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
