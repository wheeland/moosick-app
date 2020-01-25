#include "stringmodel.hpp"

#include <QTimer>

static QStringList splitIntoKeywords(const QString &string)
{
    QStringList keywords;
    for (const QString &kw : string.split(" ")) {
        if (!kw.isEmpty())
            keywords << kw.toLower();
    }
    return keywords;
}

bool StringModel::Entry::matchesKeywords(const QStringList &kws) const
{
    for (const QString &kw : kws) {
        bool hasKw = false;
        for (const QString &myKw : keywords) {
            if (myKw.contains(kw)) {
                hasKw = true;
                break;
            }
        }

        if (!hasKw)
            return false;
    }

    return true;
}

int StringModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_filteredEntries.size();
}

QVariant StringModel::data(const QModelIndex &index, int role) const
{
    const int idx = index.row();

    if (idx < 0 || idx >= rowCount(QModelIndex())) {
        return QVariant();
    }

    switch (role) {
    case TextRole: return m_filteredEntries[idx].string;
    case IdRole: return m_filteredEntries[idx].id;
    case SelectedRole: return QVariant::fromValue(m_selection == idx);
    }

    return QVariant();
}

QHash<int, QByteArray> StringModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[TextRole] = "text";
    roles[IdRole] = "id";
    roles[SelectedRole] = "selected";
    return roles;
}

void StringModel::add(int id, const QString &string)
{
    m_newEntries << Entry{ id, string, splitIntoKeywords(string) };
    m_dirty = true;
    QTimer::singleShot(0, this, &StringModel::endEditing);
}

void StringModel::clear()
{
    m_newEntries.clear();
    m_dirty = true;
    QTimer::singleShot(0, this, &StringModel::endEditing);
}

void StringModel::endEditing()
{
    if (!m_dirty)
        return;

    m_selection = -1;
    m_entries = m_newEntries;

    // sort alphabetically
    qSort(m_entries.begin(), m_entries.end(), [=](const Entry &lhs, const Entry &rhs) {
        const int count = qMin(lhs.keywords.size(), rhs.keywords.size());
        for (int i = 0; i < count; ++i) {
            const int cmp = lhs.keywords[i].compare(rhs.keywords[i]);
            if (cmp < 0)
                return true;
            else if (cmp > 0)
                return false;
        }
        return lhs.keywords.size() <= rhs.keywords.size();
    });

    updateFilter();
}

void StringModel::updateFilter()
{
    beginResetModel();

    const QStringList keywords = splitIntoKeywords(m_manualString);

    if (keywords.isEmpty()) {
        m_filteredEntries = m_entries;
    }
    else {
        m_filteredEntries.clear();

        for (const Entry &entry : m_entries) {
            if (entry.matchesKeywords(keywords))
                m_filteredEntries << entry;
        }
    }

    endResetModel();
}

void StringModel::doSelect(int idx)
{
    if (m_selection != idx) {
        m_selection = idx;
        Q_EMIT dataChanged(index(0), index(m_filteredEntries.size() - 1), QVector<int>{ /*SelectedRole*/ });

        emit selected(selectedId());
    }
}

int StringModel::selectedId() const
{
    return (m_selection >= 0 && m_selection < m_filteredEntries.size())
            ? m_filteredEntries[m_selection].id
            : -1;
}

QString StringModel::enteredString() const
{
    return (m_selection >= 0 && m_selection < m_filteredEntries.size())
            ? m_filteredEntries[m_selection].string
            : m_manualString;
}

void StringModel::select(int id)
{
    doSelect(qBound(0, id, m_entries.size() - 1));
}

void StringModel::entered(const QString &string)
{
    m_manualString = string;

    updateFilter();

    // see if this selects something
    int selection = -1;
    for (int i = 0; i < m_filteredEntries.size(); ++i) {
        if (m_filteredEntries[i].string == string) {
            selection = i;
        }
    }

    doSelect(selection);
}
