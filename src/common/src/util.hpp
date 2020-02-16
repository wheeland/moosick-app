#pragma once

#include <QProcess>

#include "library.hpp"
#include "messages.hpp"

namespace ClientCommon {

bool sendChanges(
        const ServerConfig &config,
        const QVector<Moosick::LibraryChange> &changes,
        QVector<Moosick::CommittedLibraryChange> &answers
);

int runProcess(
        const QString &program,
        const QStringList &args,
        QByteArray *out,
        QByteArray *err,
        int timeout
);

} //namespace ClientCommon
