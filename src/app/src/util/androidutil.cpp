#include "androidutil.hpp"

#include <QEvent>

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

static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(ctx)
    s_handlingLogger->add(msg);
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
    beginInsertRows(QModelIndex(), 0, 0);
    m_lines.prepend(line);
    endInsertRows();

    if (m_lines.size() > m_lineCount) {
        beginRemoveRows(QModelIndex(), m_lineCount, m_lines.size() - 1);
        m_lines.resize(m_lineCount);
        endRemoveRows();
    }
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
