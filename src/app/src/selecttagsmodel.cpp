#include "selecttagsmodel.hpp"
#include "database/database_items.hpp"

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

void SelectTagsModel::clear()
{
    m_lastSelectedFlag.clear();
    for (const TagEntry &entry : m_tagEntries.data())
        m_lastSelectedFlag[entry.tag->id()] = entry.selected;

    m_tagEntries.clear();
    m_rootTags.clear();
}

void SelectTagsModel::addTag(DbTag *tag)
{
    if (tag->parentTag()) {
        addTag(tag->parentTag());
        return;
    }

    if (!m_rootTags.contains(tag)) {
        m_rootTags << tag;
        if (!m_dirty) {
            m_dirty = true;
            QTimer::singleShot(0, this, &SelectTagsModel::updateEntries);
        }
    }
}

// sort root tags alphabetically
void sortTags(QVector<DbTag*> &tags)
{
    std::sort(tags.begin(), tags.end(), [](DbTag *lhs, DbTag *rhs) {
        return lhs->name().toLower() <= rhs->name().toLower();
    });
}

void SelectTagsModel::addTagEntry(Database::DbTag *tag, int offset)
{
    const bool wasSelected = m_lastSelectedFlag.value(tag->id(), false);
    m_tagEntries.add(TagEntry{ tag, wasSelected, offset });

    QVector<DbTag*> childTags = tag->childTags();
    sortTags(childTags);
    for (DbTag *child : childTags)
        addTagEntry(child, offset + 1);
}

void SelectTagsModel::updateEntries()
{
    if (!m_dirty)
        return;
    m_dirty = false;

    m_tagEntries.clear();

    sortTags(m_rootTags);

    for (DbTag *rootTag : m_rootTags)
        addTagEntry(rootTag, 0);
}

void SelectTagsModel::setSelected(DbTag *tag, bool selected)
{
    bool changed = false;

    for (int i = 0; i < m_tagEntries.size(); ++i) {
        TagEntry entry = m_tagEntries[i];

        if (entry.tag == tag) {
            entry.selected = selected;
            m_tagEntries.set(i, entry);
            changed = true;
        }
        else if (entry.selected && !m_multiSelect) {
            entry.selected = false;
            m_tagEntries.set(i, entry);
            changed = true;
        }
    }

    if (changed)
        emit selectionChanged();
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

Moosick::TagIdList SelectTagsModel::selectedTagsIds() const
{
    Moosick::TagIdList ret;
    for (const TagEntry &entry : m_tagEntries.data()) {
        if (entry.selected)
            ret << entry.tag->id();
    }
    return ret;
}

void SelectTagsModel::setSelectedTagIds(const Moosick::TagIdList &tags, bool multiSelect)
{
    Q_ASSERT(tags.size() <= 1 || multiSelect);

    for (int i = 0; i < m_tagEntries.size(); ++i) {
        TagEntry entry = m_tagEntries[i];

        // remove selected flag?
        if (entry.selected && !tags.contains(entry.tag->id())) {
            entry.selected = false;
            m_tagEntries.set(i, entry);
        }
        // add selected flag?
        else if (!entry.selected && tags.contains(entry.tag->id())) {
            entry.selected = true;
            m_tagEntries.set(i, entry);
        }
    }

    m_multiSelect = multiSelect;
}
