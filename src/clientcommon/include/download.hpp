#pragma once

#include "library.hpp"
#include "messages.hpp"
#include "json.hpp"
#include "requests.hpp"

#include <QTcpSocket>

namespace ClientCommon {

QVector<Moosick::CommittedLibraryChange> bandcampDownload(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir);

QVector<Moosick::CommittedLibraryChange> youtubeDownload(
        const ServerConfig &server,
        const NetCommon::DownloadRequest &request,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir);

} //namespace ClientCommon
