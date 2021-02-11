#pragma once

#include <QTcpServer>

#include "tcpclientserver.hpp"
#include "library.hpp"
#include "library_messages.hpp"

class Server : public TcpServer
{
public:
    Server();
    ~Server();

    JsonifyError init(const QString &libraryPath,
                      const QString &logPath,
                      const QString &backupBasePath);

protected:
    QByteArray handleMessage(const QByteArray &data) override;

private:
    void saveLibrary() const;

    QString m_libraryPath;
    QString m_logPath;
    QString m_backupBasePath;

    Moosick::Library m_library;
};
