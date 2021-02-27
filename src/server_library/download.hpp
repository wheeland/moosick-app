#pragma once

#include "library.hpp"
#include "library_messages.hpp"
#include "library_messages.hpp"

#include <QTcpSocket>

struct DownloadResult
{
    struct File
    {
        QString fullPath;
        QString fileEnding;
        QString title;
        int albumPosition;
        int duration;
    };
    QVector<File> files;
    QString artistName;
    quint32 artistId;
    QString albumName;
    QString tempDir;
};

Result<DownloadResult, QString> download(
        const MoosickMessage::DownloadRequest &downloadRequest,
        const QString &toolDir,
        const QString &tempDir);
