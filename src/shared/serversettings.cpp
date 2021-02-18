#include "serversettings.hpp"

#include <QDebug>

template <class T, class SettingsClass>
T convert(const SettingsClass &v, bool &valid) { Q_UNUSED(valid); return v; }

template <>
QtMsgType convert<QtMsgType, QString>(const QString &str, bool &valid)
{
    if (str == "info")
        return QtInfoMsg;
    if (str == "debug")
        return QtDebugMsg;
    if (str == "warning")
        return QtWarningMsg;
    if (str == "critical")
        return QtCriticalMsg;
    if (str == "fatal")
        return QtFatalMsg;
    qWarning() << "Invalid log level. Valid values include: info, debug, warning, fatal";
    valid = false;
    return QtFatalMsg;
}

template <class T, class SettingsClass = T>
static T getOrCreate(QSettings &settings, bool &valid, const char *name)
{
    if (!settings.contains(name)) {
        settings.setValue(name, {});
        qWarning() << "ServerSettings:" << name << "not set in" << settings.fileName();
        valid = false;
    }
    const SettingsClass ret = settings.value(name).value<SettingsClass>();
    if (ret == SettingsClass()) {
        qWarning() << "ServerSettings:" << name << "is empty in" << settings.fileName();
        valid = false;
    }

    return convert<T, SettingsClass>(ret, valid);
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

    m_cgiLogFile = getOrCreate<QString>(*settings, m_valid, "CGI_LOG_FILE");
    m_dbserverLogFile = getOrCreate<QString>(*settings, m_valid, "DBSERVER_LOG_FILE");
    m_downloaderLogFile = getOrCreate<QString>(*settings, m_valid, "DOWNLOADER_LOG_FILE");

    m_cgiLogLevel = getOrCreate<QtMsgType, QString>(*settings, m_valid, "CGI_LOG_LEVEL");
    m_dbserverLogLevel = getOrCreate<QtMsgType, QString>(*settings, m_valid, "DBSERVER_LOG_LEVEL");
    m_downloaderLogLevel = getOrCreate<QtMsgType, QString>(*settings, m_valid, "DOWNLOADER_LOG_LEVEL");

    delete settings;
}

