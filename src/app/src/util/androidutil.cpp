#include "androidutil.hpp"

#include <QEvent>
#include <QTimer>

namespace AndroidUtil {

bool CloseFilter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

    if (event->type() == QEvent::Close) {
        emit backButtonPressed();
        return true;
    }
    return false;
}

static Logger *s_handlingLogger = nullptr;

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

static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(ctx)
    const QDateTime dt = QDateTime::currentDateTime();
    const QString dtStr = QString::asprintf("%02d:%02d:%02d.%03d",
                                            dt.time().hour(),
                                            dt.time().minute(),
                                            dt.time().second(),
                                            dt.time().msec());
    const QString out = dtStr + " " + QString(msgTypeStr(type)) + " " + msg;
    fprintf(stderr, "%s\n", out.toLocal8Bit().data());
    fflush(stderr);
    s_handlingLogger->add(out);
}

void Logger::install()
{
    s_handlingLogger = this;
    qInstallMessageHandler(&messageHandler);
}

void Logger::uninstall()
{
    qInstallMessageHandler(nullptr);
}

int Logger::lineCount() const
{
    return m_lineCount;
}

void Logger::setLineCount(int lineCount)
{
    if (m_lineCount == lineCount)
        return;

    beginResetModel();
    m_lineCount = lineCount;
    while (m_lines.size() > m_lineCount)
        m_lines.removeFirst();
    endResetModel();
    emit lineCountChanged(m_lineCount);
}

void Logger::add(const QString &line)
{
    QTimer::singleShot(0, this, [=]() {
        beginInsertRows(QModelIndex(), 0, 0);
        m_lines.append(line);
        endInsertRows();

        while (m_lines.size() > m_lineCount) {
            beginRemoveRows(QModelIndex(), 0, 0);
            m_lines.removeFirst();
            endRemoveRows();
        }
    });
}

int Logger::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_lines.size();
}

QVariant Logger::data(const QModelIndex &index, int role) const
{
    const int idx = index.row();

    if (idx < 0 || idx >= rowCount(QModelIndex())) {
        return QVariant();
    }

    switch (role) {
    case TextRole:
        return QVariant::fromValue(m_lines[idx]);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> Logger::roleNames() const
{
    QHash<int, QByteArray> ret;
    ret[TextRole] = "text";
    return ret;
}

} // namespace AndroidUtil
