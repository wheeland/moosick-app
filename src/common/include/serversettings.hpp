#pragma once

#include <QSettings>

class ServerSettings
{
public:
    ServerSettings();
    ~ServerSettings() = default;

    bool isValid() const { return m_valid; }

    QString serverRoot() const { return m_serverRoot; }
    QString tempDir() const { return m_tempDir; }
    QString toolsDir() const { return m_toolsDir; }

    QString libraryFile() const { return m_libraryFile; }
    QString libraryLogFile() const { return m_libraryLogFile; }
    QString libraryBackupDir() const { return m_libraryBackupDir; }

    quint16 downloaderPort() const { return m_downloaderPort; }
    quint16 dbserverPort() const { return m_dbserverPort; }

private:
    bool m_valid;

    QString m_serverRoot;
    QString m_tempDir;
    QString m_toolsDir;

    QString m_libraryFile;
    QString m_libraryLogFile;
    QString m_libraryBackupDir;

    quint16 m_downloaderPort;
    quint16 m_dbserverPort;
};
