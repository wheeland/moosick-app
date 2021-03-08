#include "mainwindow.hpp"

#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>

#include <taglib/fileref.h>

static void removePrefix(QString &dst, const QString &prefix)
{
    if (dst.startsWith(prefix))
        dst = dst.mid(prefix.size());
}

const QStringList s_audioSuffixes = {
    "3gp",
    "flac",
    "m4a",
    "mp3",
    "ogg", "oga", "mogg",
    "opus",
    "wav",
    "wma",
    "webm"
};

MainWindow::MainWindow(Connection *connection, QWidget *parent)
    : QMainWindow(parent)
    , m_connection(connection)
    , m_fileView(new FileView())
{
    connect(m_connection, &Connection::networkReplyFinished, this, &MainWindow::onNetworkReplyFinished);

    QWidget *centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    QVBoxLayout *centralVBoxLayout = new QVBoxLayout(centralWidget);
    centralVBoxLayout->addWidget(m_fileView);

    QWidget *bottomPane = new QWidget();
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomPane->setLayout(bottomLayout);
    centralVBoxLayout->addWidget(bottomPane);

    bottomLayout->addStretch();

    QPushButton *addFileButton = new QPushButton("Add files");
    connect(addFileButton, &QPushButton::clicked, this, &MainWindow::onAddFileClicked);
    bottomLayout->addWidget(addFileButton);
    bottomLayout->addStretch();

    QPushButton *addDirButton = new QPushButton("Add directories");
    connect(addDirButton, &QPushButton::clicked, this, &MainWindow::onAddDirClicked);
    bottomLayout->addWidget(addDirButton);
    bottomLayout->addStretch();

    QPushButton *uploadButton = new QPushButton("Beam 'em up, Scotty!");
    connect(uploadButton, &QPushButton::clicked, this, &MainWindow::onUploadClicked);
    bottomLayout->addWidget(uploadButton);
    bottomLayout->addStretch();
}

MainWindow::~MainWindow()
{

}

void MainWindow::onAddFileClicked()
{
    QFileDialog dialog(this, "Select files");
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setViewMode(QFileDialog::List);
    if (dialog.exec()) {
        const QStringList files = dialog.selectedFiles();
        const QFileInfo baseDir(dialog.directory().path());
        QTimer::singleShot(0, this, [=]() { addFilesToTableView(baseDir.absoluteFilePath(), files); });
    }
}

void MainWindow::onAddDirClicked()
{
    QFileDialog dialog(this, "Select directories");
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setViewMode(QFileDialog::List);

    QStringList files;

    if (dialog.exec()) {
        const QStringList dirs = dialog.selectedFiles();
        const QFileInfo baseDir(dialog.directory().path());
        for (const QString &dir : dirs)
            collectAudioFiles(dir, files);
        QTimer::singleShot(0, this, [=]() { addFilesToTableView(baseDir.absoluteFilePath(), files); });
    }
}

void MainWindow::collectAudioFiles(const QString &path, QStringList &dst)
{
    if (!QFileInfo(path).isDir())
        return;

    const QFileInfoList entries = QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir())
            collectAudioFiles(entry.filePath(), dst);
        else if (entry.isFile())
            dst << entry.filePath();
    }
}

void MainWindow::addFilesToTableView(const QString &baseDir, const QStringList &files)
{
    QVector<QFileInfo> filtered;

    for (const QString &file : files) {
        const QFileInfo fileInfo(file);
        const bool isAudioFile = fileInfo.isFile() && s_audioSuffixes.contains(fileInfo.suffix());
        if (isAudioFile)
            filtered << fileInfo;
    }

    QProgressDialog progress("Adding files...", "Cancel", 0, filtered.size(), this);
    progress.setWindowModality(Qt::WindowModal);

    for (int i = 0; i < filtered.size(); ++i) {
        const TagLib::FileRef file(filtered[i].absoluteFilePath().toUtf8().data());
        const TagLib::String artist = file.tag()->artist();
        const TagLib::String album = file.tag()->album();
        const TagLib::String title = file.tag()->title();
        const int position = file.tag()->track();
        const TagLib::AudioProperties *audio = file.audioProperties();
        const int duration = audio ? audio->length() : 0;

        const QString absPath = filtered[i].absoluteFilePath();
        QString displayPath = absPath;
        removePrefix(displayPath, baseDir);
        removePrefix(displayPath, "/");
        removePrefix(displayPath, "\\");

        const FileEntry fileEntry{
            absPath,
            displayPath,
            QString::fromStdString(artist.to8Bit(true)),
            QString::fromStdString(album.to8Bit(true)),
            position,
            QString::fromStdString(title.to8Bit(true)),
            duration
        };

        progress.setValue(i);
        if (progress.wasCanceled())
            break;

        m_fileView->add(fileEntry);
    }
}

void MainWindow::startNextUpload()
{
    m_uploadProgress->setValue(m_uploadProgress->value() + 1);

    Q_ASSERT(!m_currentUpload);
    if (m_filesToUpload.isEmpty()) {
        m_uploadProgress.reset();
        return;
    }

    const FileEntry fileEntry = m_filesToUpload.takeFirst();

    // check for sanity of this entry
    if (fileEntry.artist.isEmpty()
            || fileEntry.album.isEmpty()
            || fileEntry.title.isEmpty()
            || fileEntry.duration <= 0
            || fileEntry.position < 0
            || !QFileInfo(fileEntry.path).exists()
            || !QFileInfo(fileEntry.path).isReadable()
            || QFileInfo(fileEntry.path).size() > (90 << 20)) {
        QString message = "Invalid file, skipping upload:\n\n";
        message += "Artist: " + fileEntry.artist + "\n";
        message += "Album: " + fileEntry.album + "\n";
        message += "Title: " + fileEntry.title + "\n";
        message += "duration: " + QString::number(fileEntry.duration) + " sec\n";
        message += "position: " + QString::number(fileEntry.position) + "\n";
        message += "file: " + fileEntry.displayPath + " (" + QString::number(QFileInfo(fileEntry.path).size()) + " bytes)";
        auto ret = QMessageBox::question(this, "Skipping upload", message, QMessageBox::Ignore, QMessageBox::Cancel);

        if (ret == QMessageBox::Ignore)
            startNextUpload();
        else if (ret == QMessageBox::Cancel)
            cancelUploads();

        return;
    }

    QFile file(fileEntry.path);
    if (!file.open(QIODevice::ReadOnly))
        startNextUpload();

    MoosickMessage::UploadSongRequest request;
    request.title = fileEntry.title;
    request.duration = fileEntry.duration;
    request.position = fileEntry.position;
    request.albumName = fileEntry.album;
    request.artistName = fileEntry.artist;
    request.fileEnding = QFileInfo(fileEntry.path).suffix();
    request.fileSize = QFileInfo(fileEntry.path).size();

    const QByteArray msgJson = MoosickMessage::Message(request).toJson();
    const QByteArray msg64 = msgJson.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    m_currentUpload = m_connection->post(file.readAll(), "message64=" + msg64);
}

void MainWindow::cancelUploads()
{
    m_filesToUpload.clear();
    m_uploadProgress.reset();
}

void MainWindow::onNetworkReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    qWarning() << "Received response" << reply << m_currentUpload;

    if (m_currentUpload == reply) {
        m_currentUpload = nullptr;

        auto result = m_connection->tryParseReply(reply);
        if (result.hasError()) {
            qWarning() << result.getError();
            startNextUpload();
            return;
        }

        if (result->getType() == MoosickMessage::Type::Error) {
            qWarning() << result->as<MoosickMessage::Error>()->errorMessage;
            startNextUpload();
            return;
        }

        const MoosickMessage::UploadSongResponse *response = result->as<MoosickMessage::UploadSongResponse>();
        qWarning() << "Created SongID:" << response->songId;
        startNextUpload();
    }
}

void MainWindow::onUploadClicked()
{
    if (m_currentUpload)
        return;

    m_filesToUpload = m_fileView->files();

    m_uploadProgress.reset(new QProgressDialog(this));
    m_uploadProgress->setWindowModality(Qt::WindowModal);
    m_uploadProgress->hide();
    m_uploadProgress->setRange(0, m_filesToUpload.size());
    m_uploadProgress->setLabelText("Uploading " + QString::number(m_filesToUpload.size()) + " files...");
    m_uploadProgress->setCancelButtonText("Cancel");
    m_uploadProgress->setValue(0);
    m_uploadProgress->show();

    startNextUpload();
}
