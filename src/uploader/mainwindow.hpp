#pragma once

#include <QMainWindow>
#include <QProgressDialog>

#include "fileview.hpp"
#include "connection.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Connection *connection, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAddFileClicked();
    void onAddDirClicked();
    void onUploadClicked();
    void onNetworkReplyFinished(QNetworkReply *reply);

private:
    Connection *m_connection;
    FileView *m_fileView;

    static void collectAudioFiles(const QString &path, QStringList &dst);

    void addFilesToTableView(const QString &baseDir, const QStringList &files);

    QNetworkReply *m_currentUpload = nullptr;
    QVector<FileEntry> m_filesToUpload;
    QScopedPointer<QProgressDialog> m_uploadProgress;

    void startNextUpload();
    void cancelUploads();
};
