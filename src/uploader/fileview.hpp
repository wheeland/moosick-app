#pragma once

#include <QTableView>
#include <QAbstractTableModel>

struct FileEntry
{
    QString path;
    QString displayPath;
    QString artist;
    QString album;
    int position;
    QString title;
    int duration;

    enum Type {
        Path,
        Artist,
        Album,
        Position,
        Title,
        Duration,
        Count
    };
};

class FileModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    FileModel(QObject *parent = nullptr);
    ~FileModel() = default;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void sort(int column, Qt::SortOrder order) override;

private:
    void addFile(const FileEntry &file);
    void removeFile(int index);

    QVector<FileEntry> m_files;

    friend class FileView;
};

class FileView : public QTableView
{
    Q_OBJECT

public:
    FileView(QWidget *parent = nullptr);
    ~FileView();

    void add(const FileEntry &fileEntry);

    QVector<FileEntry> files() const { return m_model->m_files; }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    bool interceptKeyPress(QKeyEvent *event);
    
    FileModel *m_model;
};
