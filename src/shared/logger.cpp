#include "logger.hpp"

#include <QDateTime>
#include <iostream>

QFile Logger::ms_logFile;
QString Logger::ms_logFilePath;
QtMsgType Logger::ms_stderrFilter = QtWarningMsg;
QtMsgType Logger::ms_logfileFilter = QtDebugMsg;

static const char *msgTypeStr(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "[Dbg]";
    case QtWarningMsg: return "[Wrn]";
    case QtCriticalMsg: return "[Crt]";
    case QtFatalMsg: return "[Ftl]";
    case QtInfoMsg: return "[Inf]";
    default: return "[xxx]";
    }
}

static bool printMessage(QtMsgType type, QtMsgType filter)
{
    switch (filter) {
    case QtInfoMsg: return true;
    case QtDebugMsg: return (type != QtInfoMsg);
    case QtWarningMsg: return (type != QtInfoMsg) && (type != QtDebugMsg);
    case QtCriticalMsg: return (type == QtFatalMsg) || (type == QtCriticalMsg);
    case QtFatalMsg: return (type == QtFatalMsg);
    default: return false;
    }
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(ctx)

    const bool printStderr = printMessage(type, ms_stderrFilter);
    const bool printLogfile = printMessage(type, ms_logfileFilter);

    if (!printStderr && !printLogfile)
        return;

    const QDateTime dt = QDateTime::currentDateTime();
    const QString dtStr = QString::asprintf(" %2d-%02d-%02d %02d:%02d:%02d:%03d ",
             dt.date().year() % 100, dt.date().month(), dt.date().day(),
             dt.time().hour(), dt.time().minute(), dt.time().second(), dt.time().msec());

    const QString out = msgTypeStr(type) + dtStr + msg;
    const QByteArray utf8 = out.toUtf8();

    if (printLogfile) {
        ms_logFile.write(utf8);
        ms_logFile.putChar('\n');
        ms_logFile.flush();
    }
    if (printStderr) {
        std::cerr << utf8.constData() << std::endl;
    }
}

void Logger::install()
{
    qInstallMessageHandler(&messageHandler);
}

void Logger::uninstall()
{
    qInstallMessageHandler(nullptr);
}

void Logger::setLogFile(const QString &path)
{
    ms_logFilePath = path;
    ms_logFile.setFileName(path);
    if (ms_logFile.exists())
        ms_logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    else
        ms_logFile.open(QIODevice::WriteOnly);
}

void Logger::setStderrFilter(QtMsgType firstVisibleCategory)
{
    ms_stderrFilter = firstVisibleCategory;
}

void Logger::setLogFileFilter(QtMsgType firstVisibleCategory)
{
    ms_logfileFilter = firstVisibleCategory;
}
