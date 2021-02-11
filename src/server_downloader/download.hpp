#pragma once

#include "library.hpp"
#include "library_messages.hpp"
#include "library_messages.hpp"

#include <QTcpSocket>

Result<QVector<Moosick::CommittedLibraryChange>, QString> download(
        QTcpSocket &tcpSocket,
        const MoosickMessage::DownloadRequest &downloadRequest,
        const QString &mediaDir,
        const QString &toolDir,
        const QString &tempDir);
