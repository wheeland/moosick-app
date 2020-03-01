#pragma once

#include "util/modeladapter.hpp"
#include "library.hpp"
#include "flatmap.hpp"

namespace Database {
class DbItem;
class DbTag;
class DbArtist;
class DbAlbum;
class DbSong;
}

class SelectTagsModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ModelAdapter::Model *tags READ tags CONSTANT)

public:
    SelectTagsModel(QObject *parent = nullptr);
    ~SelectTagsModel() override;

    ModelAdapter::Model *tags() const { return m_tagEntries.model(); }

    void clear();
    void addTag(Database::DbTag *tag);

    Q_INVOKABLE void setSelected(Database::DbTag *tag, bool selected);

    QVector<Database::DbTag*> selectedTags() const;
    Moosick::TagIdList selectedTagsIds() const;

    void setSelectedTagIds(const Moosick::TagIdList &tags, bool multiSelect);

public slots:
    void updateEntries();

signals:
    void selectionChanged();

private:
    bool m_dirty = false;
    bool m_multiSelect = true;
    QVector<Database::DbTag*> m_rootTags;

    void addTagEntry(Database::DbTag *tag, int offset);

    struct TagEntry {
        Database::DbTag *tag;
        bool selected;
        int offset;
    };
    ModelAdapter::Adapter<TagEntry> m_tagEntries;

    // caches the selected flag when the model was cleared, so that we can restore it later
    FlatMap<Moosick::TagId, bool> m_lastSelectedFlag;
};
