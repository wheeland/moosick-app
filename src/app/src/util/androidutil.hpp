#pragma once

#include <QObject>
#include <QStringList>
#include <QAbstractListModel>
#include <QDateTime>

namespace AndroidUtil {

/**
 * Emits a signal every time the back button is pressed on Android
 */
class CloseFilter : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~CloseFilter() override = default;

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void backButtonPressed();
};

/**
 * Hooks into the Qt logger and presents the last N log outputs to QML
 */
class Logger : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int lineCount READ lineCount WRITE setLineCount NOTIFY lineCountChanged)

    enum Role {
        TextRole = Qt::UserRole + 1
    };

public:
    using QAbstractListModel::QAbstractListModel;
    ~Logger() override = default;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void install();
    void uninstall();

    void add(const QString &line);

    void setLineCount(int lineCount);
    int lineCount() const;

signals:
    void lineCountChanged(int lineCount);

private:
    QVector<QString> m_lines;
    int m_lineCount = 256;
};

} // namespace AndroidUtil
