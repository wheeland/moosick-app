#pragma once

#include <QDebug>
#include <QString>
#include <QFile>

/**
 * Hooks into the Qt logger and prints output to stderr and/or to a logfile
 */
class Logger
{
public:
    static void install();
    static void uninstall();

    static void setLogFile(const QString &path);
    static void setStderrFilter(QtMsgType firstVisibleCategory);
    static void setLogFileFilter(QtMsgType firstVisibleCategory);

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);

    static QString ms_logFilePath;
    static QFile ms_logFile;
    static QtMsgType ms_stderrFilter;
    static QtMsgType ms_logfileFilter;
};
