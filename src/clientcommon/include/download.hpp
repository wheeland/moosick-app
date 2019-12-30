#pragma once

#include "library.hpp"
#include "messages.hpp"
#include "json.hpp"
#include "requests.hpp"

#include <QTcpSocket>

namespace ClientCommon {

QVector<Moosick::LibraryChange> bandcampDownload(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir);

} //namespace ClientCommon
