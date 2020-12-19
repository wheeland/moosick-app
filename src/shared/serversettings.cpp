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

    m_serverRoot = getOrCreate<QString>(*settings, m_valid, "SERVER_ROOT");
    m_tempDir = getOrCreate<QString>(*settings, m_valid, "TEMP_DIR");
    m_toolsDir = getOrCreate<QString>(*settings, m_valid, "TOOLS_DIR");

    m_libraryFile = getOrCreate<QString>(*settings, m_valid, "LIBRARY_FILE");
    m_libraryLogFile = getOrCreate<QString>(*settings, m_valid, "LIBRARY_LOG_FILE");
    m_libraryBackupDir = getOrCreate<QString>(*settings, m_valid, "LIBRARY_BACKUP_DIR");

    m_downloaderPort = getOrCreate<quint16>(*settings, m_valid, "DOWNLOADER_PORT");
    m_dbserverPort = getOrCreate<quint16>(*settings, m_valid, "DBSERVER_PORT");

    delete settings;
}

