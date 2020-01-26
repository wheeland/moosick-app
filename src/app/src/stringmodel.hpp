#pragma once

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QAbstractListModel>

class StringModel : public QAbstractListModel
{
    Q_OBJECT

    enum Role {
        TextRole = Qt::UserRole + 1,
        IdRole,
        SelectedRole,
    };

public:
    using QAbstractListModel::QAbstractListModel;
    ~StringModel() override = default;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void add(int id, const QString &string);
    void clear();

    int selectedId() const;
    QString enteredString() const;

    Q_INVOKABLE void entered(const QString &string);

private slots:
    void endEditing();
    void updateFilter();

signals:
    void popup(const QString &initialString);
    void selected(int id);

private:
    void doSelect(int idx);

    struct Entry {
        int id;
        QString string;
        QStringList keywords;

        bool matchesKeywords(const QStringList &kws) const;
    };

    // source data model:
    bool m_dirty = false;
    QVector<Entry> m_newEntries;
    QVector<Entry> m_entries;

    // filtered model:
    QVector<Entry> m_filteredEntries;

    int m_selection = 0;
    QString m_manualString;
};
