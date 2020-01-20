#include "selecttagsmodel.hpp"
#include "database_items.hpp"

#include <QTimer>

using Database::DbTag;

SelectTagsModel::SelectTagsModel(QObject *parent)
    : QObject(parent)
{
    m_tagEntries.addAccessor("tag", [](const TagEntry &entry) { return QVariant::fromValue(entry.tag); });
    m_tagEntries.addAccessor("selected", [](const TagEntry &entry) { return QVariant::fromValue(entry.selected); });
    m_tagEntries.addAccessor("offset", [](const TagEntry &entry) { return QVariant::fromValue(entry.offset); });
}

SelectTagsModel::~SelectTagsModel()
{
}

void SelectTagsModel::addTag(DbTag *tag)
{
    if (tag->parentTag()) {
        addTag(tag->parentTag());
        return;
    }

    if (!m_rootTags.contains(tag)) {
        m_rootTags << tag;
        m_dirty = true;
        QTimer::singleShot(0, this, &SelectTagsModel::updateEntries);
    }
}

// sort root tags alphabetically
void sortTags(QVector<DbTag*> &tags)
{
    qSort(tags.begin(), tags.end(), [](DbTag *lhs, DbTag *rhs) {
        return lhs->name().toLower() <= rhs->name().toLower();
    });
};

void SelectTagsModel::addTagEntry(Database::DbTag *tag, int offset)
{
    m_tagEntries.add(TagEntry{ tag, false, offset });

    QVector<DbTag*> childTags = tag->childTags();
    sortTags(childTags);
    for (DbTag *child : childTags)
        addTagEntry(child, offset + 1);
}

void SelectTagsModel::updateEntries()
{
    m_tagEntries.clear();

    sortTags(m_rootTags);

    for (DbTag *rootTag : m_rootTags)
        addTagEntry(rootTag, 0);
}

void SelectTagsModel::setSelected(DbTag *tag, bool selected)
{
    for (int i = 0; i < m_tagEntries.size(); ++i) {
        TagEntry entry = m_tagEntries[i];
        if (entry.tag == tag) {
            entry.selected = selected;
            m_tagEntries.set(i, entry);
            return;
        }
    }
}

QVector<DbTag*> SelectTagsModel::selectedTags() const
{
    QVector<DbTag*> ret;
    for (const TagEntry &entry : m_tagEntries.data()) {
        if (entry.selected)
            ret << entry.tag;
    }
    return ret;
}
