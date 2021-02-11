#include "serversettings.hpp"

#include <QDebug>

template <class T>
static T getOrCreate(QSettings &settings, bool &valid, const char *name)
{
    if (!settings.contains(name)) {
        settings.setValue(name, T());
        qWarning() << "ServerSettings:" << name << "not set in" << settings.fileName();
        valid = false;
    }
    const T ret = settings.value(name).value<T>();
    if (ret == T()) {
        qWarning() << "ServerSettings:" << name << "is empty in" << settings.fileName();
        valid = false;
    }

    return ret;
}

ServerSettings::ServerSettings()
{
    const QString serverSettingsFileEnv = qgetenv("SERVER_SETTINGS_FILE");

    QSettings *settings = (serverSettingsFileEnv.isEmpty())
            ? new QSettings(QSettings::IniFormat, QSettings::UserScope, "de.wheeland.moosick", "MoosickServer")
            : new QSettings(serverSettingsFileEnv, QSettings::IniFormat);

    m_valid = true;

    m_mediaBaseUrl = getOrCreate<QString>(*settings, m_valid, "MEDIA_BASE_URL");
    m_mediaBaseDir = getOrCreate<QString>(*settings, m_valid, "MEDIA_BASE_DIR");

    m_tempDir = getOrCreate<QString>(*settings, m_valid, "TEMP_DIR");
    m_toolsDir = getOrCreate<QString>(*settings, m_valid, "TOOLS_DIR");

    m_libraryFile = getOrCreate<QString>(*settings, m_valid, "LIBRARY_FILE");
    m_libraryLogFile = getOrCreate<QString>(*settings, m_valid, "LIBRARY_LOG_FILE");
    m_libraryBackupDir = getOrCreate<QString>(*settings, m_valid, "LIBRARY_BACKUP_DIR");

    m_downloaderPort = getOrCreate<quint16>(*settings, m_valid, "DOWNLOADER_PORT");
    m_dbserverPort = getOrCreate<quint16>(*settings, m_valid, "DBSERVER_PORT");

    m_downloaderHost = getOrCreate<QString>(*settings, m_valid, "DOWNLOADER_HOST");
    m_dbserverHost = getOrCreate<QString>(*settings, m_valid, "DBSERVER_HOST");

    m_cgiLogFile = getOrCreate<QString>(*settings, m_valid, "LOG_FILE_CGI");
    m_dbserverLogFile = getOrCreate<QString>(*settings, m_valid, "LOG_FILE_DBSERVER");
    m_downloaderLogFile = getOrCreate<QString>(*settings, m_valid, "LOG_FILE_DOWNLOADER");

    delete settings;
}

