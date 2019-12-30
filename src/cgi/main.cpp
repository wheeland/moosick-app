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
    return data.toBase64();
}

int main(int argc, char **argv)
{
    const QString request = qgetenv("REQUEST_URI");
    if (request.isEmpty() || !request.startsWith("/"))
        return 0;

    // extract command and key/value pairs
    const QStringList parts = request.mid(1).split("?");
    const QString command = parts[0];
    QHash<QString, QString> values;
    for (const QString &keyValue : parts.value(1).split("&")) {
        const QStringList parts = keyValue.split("=");
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

    QProcess::execute("/usr/bin/env");

    if (command == "lib.do") {
        ClientCommon::Message answer;
        sendRecv(s_server, ClientCommon::Message{ ClientCommon::LibraryRequest }, answer);
        std::cout << answer.data.toBase64().constData() << "\n";

        return 0;
    }
    else if (command == "search.do") {
        std::cout << "[" << std::endl;

        if (values.contains("v"))
            QProcess::execute(toolDir + "node", {jsDir + "search.js", values["v"]});

        std::cout << "]" << std::endl;
        return 0;
    }
    else if (command == "bandcamp-download.do") {
        if (!values.contains("url") || !values.contains("name")
                || (!values.contains("artistId") && !values.contains("artistName")))
            return 0;

        const QString url = values["url"];
        const QString name = values["name"];
        const int artistId = values["artistId"].toInt();
        const QString artistName = values["artistName"];

        const NetCommon::DownloadRequest request { NetCommon::DownloadRequest::BandcampAlbum,
                    url, artistId, artistName, name };

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
