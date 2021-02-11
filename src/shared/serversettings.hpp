#pragma once

#include <QSettings>

class ServerSettings
{
public:
    ServerSettings();
    ~ServerSettings() = default;

    bool isValid() const { return m_valid; }

    /**
     * Base URL to all the actual media files. This URL will be
     * sent to clients who wish to access media files.
     */
    QString mediaBaseUrl() const { return m_mediaBaseUrl; }

    /**
     * Base path to the actual media files. This path is internal
     * to the server machine.
     */
    QString mediaBaseDir() const { return m_mediaBaseDir; }

    QString tempDir() const { return m_tempDir; }
    QString toolsDir() const { return m_toolsDir; }

    QString libraryFile() const { return m_libraryFile; }
    QString libraryLogFile() const { return m_libraryLogFile; }
    QString libraryBackupDir() const { return m_libraryBackupDir; }

    quint16 downloaderPort() const { return m_downloaderPort; }
    quint16 dbserverPort() const { return m_dbserverPort; }

    QString downloaderHost() const { return m_downloaderHost; }
    QString dbserverHost() const { return m_dbserverHost; }

    QString cgiLogFile() const { return m_cgiLogFile; }
    QString dbserverLogFile() const { return m_dbserverLogFile; }
    QString downloaderLogFile() const { return m_downloaderLogFile; }

private:
    bool m_valid;

    QString m_mediaBaseUrl;
    QString m_mediaBaseDir;

    QString m_tempDir;
    QString m_toolsDir;

    QString m_libraryFile;
    QString m_libraryLogFile;
    QString m_libraryBackupDir;

    quint16 m_downloaderPort;
    quint16 m_dbserverPort;

    QString m_downloaderHost;
    QString m_dbserverHost;

    QString m_cgiLogFile;
    QString m_dbserverLogFile;
    QString m_downloaderLogFile;
};
