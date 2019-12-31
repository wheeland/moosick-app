#include <QCoreApplication>
#include <QProcess>
#include <iostream>

#include "library.hpp"
#include "messages.hpp"
#include "requests.hpp"
#include "download.hpp"
#include "json.hpp"

static ClientCommon::ServerConfig s_server;

template <class T>
QByteArray toBase64(const T& value)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << value;
    return data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

int main(int argc, char **argv)
{
    const QByteArray request = qgetenv("REQUEST_URI");
    if (request.isEmpty() || !request.startsWith("/"))
        return 0;

    // extract command and key/value pairs
    const QList<QByteArray> parts = request.mid(1).split('?');
    const QByteArray command = parts[0];
    QHash<QByteArray, QByteArray> values;
    for (const QByteArray &keyValue : parts.value(1).split('&')) {
        const QList<QByteArray> parts = keyValue.split('=');
        if (parts.size() == 2)
            values[parts[0]] = parts[1];
    }

    const QString rootDir = qgetenv("DOCUMENT_ROOT");
    const QString toolDir = rootDir + "/../tools/";
    const QString jsDir = toolDir + "scrapers/";
    const QString tmpDir = "/tmp/";
    const QString mediaDir = rootDir;

    s_server.hostName = "localhost";
    s_server.port = 12345;
    s_server.timeout = 1000;

//    QProcess::execute("/usr/bin/env");

    if (command == "lib.do") {
        ClientCommon::Message answer;
        sendRecv(s_server, ClientCommon::Message{ ClientCommon::LibraryRequest }, answer);

        std::cout << answer.data.toBase64().constData() << "\n";

        return 0;
    }
    else if (command == "search.do") {
        if (values.contains("v")) {
            const QString searchPattern = QByteArray::fromBase64(values["v"], QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
            QProcess::execute(toolDir + "node", {jsDir + "search.js", searchPattern});
        } else
            std::cout << "[]" << std::endl;
        return 0;
    }
    else if (command == "bandcamp-download.do") {
        if (!values.contains("v"))
            return 0;

        NetCommon::DownloadRequest request;
        if (!request.fromBase64(values["v"])) {
            qWarning() << "Failed parsing the base64 download request";
            return 0;
        }

        const QVector<Moosick::LibraryChange> changes = ClientCommon::bandcampDownload(
                    s_server, request, mediaDir, toolDir, tmpDir);

        std::cout << toBase64(changes).data() << std::endl;
        return 0;
    }

//    std::cout << qPrintable(command) << std::endl;
//    for (auto it = values.begin(); it != values.end(); ++it)
//        std::cout << qPrintable(it.key()) << qPrintable(it.value()) << std::endl;

    return 0;
}
