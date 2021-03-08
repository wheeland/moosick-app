#include "fileview.hpp"

#include <QHeaderView>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QInputDialog>
#include <QSet>
#include <QDebug>

struct ColumnInfo {
    QString label;
    bool editable;
    float relWidth;
};

static std::array<ColumnInfo, FileEntry::Count> s_columnInfo = {
    ColumnInfo{ "File", false, 0.4f },
    ColumnInfo{ "Artist", true, 0.16f },
    ColumnInfo{ "Album", true, 0.16f },
    ColumnInfo{ "#", false, 0.03f },
    ColumnInfo{ "Title", true, 0.16f },
    ColumnInfo{ "Duration", false, 0.09f }
};

static QString encodeDuration(int secs)
{
    return QString::asprintf("%d:%02d", secs / 60, secs % 60);
}

FileModel::FileModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void FileModel::addFile(const FileEntry &file)
{
    beginInsertRows(QModelIndex(), m_files.size(), m_files.size());
    m_files << file;
    endInsertRows();
}

void FileModel::removeFile(int index)
{
    if (index < 0 || index >= m_files.size())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_files.remove(index);
    endRemoveRows();
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_files.size();
}

int FileModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return FileEntry::Count;
}

QVariant FileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < FileEntry::Count && role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return s_columnInfo[section].label;
    }
    return QVariant();
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index))
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case FileEntry::Path:
            return m_files[index.row()].displayPath;
        case FileEntry::Artist:
            return m_files[index.row()].artist;
        case FileEntry::Album:
            return m_files[index.row()].album;
        case FileEntry::Position:
            return QString::number(m_files[index.row()].position);
        case FileEntry::Title:
            return m_files[index.row()].title;
        case FileEntry::Duration:
            return encodeDuration(m_files[index.row()].duration);
        }
    }

    return QVariant();
}

bool FileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index) || role != Qt::EditRole)
        return false;

    switch (index.column()) {
    case FileEntry::Artist:
        m_files[index.row()].artist = value.toString();
        break;
    case FileEntry::Album:
        m_files[index.row()].album = value.toString();
        break;
    case FileEntry::Position:
        m_files[index.row()].position = value.toInt();
        break;
    case FileEntry::Title:
        m_files[index.row()].title = value.toString();
        break;
    }

    return false;
}

Qt::ItemFlags FileModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if (index.column() >= 1 && index.column() <= 4)
        flags |= Qt::ItemIsEditable;
    return flags;
}

void FileModel::sort(int column, Qt::SortOrder order)
{
    const auto compare = [=](const FileEntry &a, const FileEntry &b, FileEntry::Type field) {
        switch (field) {
        case FileEntry::Path: return a.path.compare(b.path, Qt::CaseInsensitive);
        case FileEntry::Artist: return a.artist.compare(b.artist, Qt::CaseInsensitive);
        case FileEntry::Album: return a.album.compare(b.album, Qt::CaseInsensitive);
        case FileEntry::Position: return (a.position - b.position);
        case FileEntry::Title: return a.title.compare(b.title, Qt::CaseInsensitive);
        case FileEntry::Duration: return (a.duration - b.duration);
        case FileEntry::Count:
            qFatal("No such FileEntry member");
        }
    };
    std::sort(m_files.begin(), m_files.end(), [=](const FileEntry &a, const FileEntry &b) {
        int col = column;
        int ret = compare(a, b, (FileEntry::Type) column);

        while (ret == 0 && col < FileEntry::Count) {
            ret = compare(a, b, (FileEntry::Type) col);
            col++;
        }

        return (order == Qt::AscendingOrder) ? (ret < 0) : (ret > 0);
    });

    dataChanged(index(0, 0), index(FileEntry::Count, m_files.size()));
}

FileView::FileView(QWidget *parent)
    : QTableView(parent)
    , m_model(new FileModel(this))
{
    setModel(m_model);
    setSortingEnabled(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

FileView::~FileView()
{

}

void FileView::add(const FileEntry &fileEntry)
{
    m_model->addFile(fileEntry);
}

void FileView::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    for (int i = 0; i < FileEntry::Count; ++i)
        setColumnWidth(i, event->size().width() * s_columnInfo[i].relWidth);
}

bool FileView::interceptKeyPress(QKeyEvent *event)
{
    if (state() != NoState)
        return false;

    const QModelIndexList selection = selectedIndexes();
    if (selection.isEmpty())
        return false;

    // Mass rename
    if (event->key() == Qt::Key_Return) {
        // let's see what we have selected...
        QSet<QString> values;
        QSet<int> cols;
        for (QModelIndex index : selection) {
            values << m_model->data(index, Qt::DisplayRole).toString();
            cols << index.column();
        }
        const QString initialValue = (values.size() == 1) ? *values.begin() : QString();

        // if we selected cells of more than one column, we will ignore this
        if (cols.size() > 1)
            return false;

        const int col = *cols.begin();
        Q_ASSERT(col < FileEntry::Count);

        if (!s_columnInfo[col].editable)
            return false;

        const QString header = s_columnInfo[col].label;
        const QString label = QString("Enter new %1 for %2 entries").arg(header).arg(selection.size());
        const QString newData = QInputDialog::getText(this, "Enter new " + header, label);
        if (!newData.isEmpty()) {
            for (QModelIndex index : selection)
                m_model->setData(index, newData, Qt::EditRole);
        }

        return true;
    }

    if (event->key() == Qt::Key_Delete) {
        // let's see what we have selected...
        QVector<int> rows;
        for (QModelIndex index : selection) {
            if (!rows.contains(index.row()))
                rows.append(index.row());
        }

        std::sort(rows.begin(), rows.end());

        for (auto it = rows.rbegin(); it != rows.rend(); ++it) {
            m_model->removeFile(*it);
        }

        return true;
    }

    return false;
}

void FileView::keyPressEvent(QKeyEvent *event)
{
    if (interceptKeyPress(event)) {
        event->ignore();
    }
    else {
        QTableView::keyPressEvent(event);
    }
}
