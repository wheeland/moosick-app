#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDataStream>
#include <QTextStream>

#include "library.hpp"
#include "messages.hpp"
#include "download.hpp"
#include "jsonconv.hpp"

static ClientCommon::ServerConfig s_serverConfig;

static const QVector<QPair<QString, int>> changeTypesAndParamCount {
    {"Invalid", 0},
    {"SongAdd", 2},
    {"SongRemove", 1},
    {"SongSetName", 2},
    {"SongSetPosition", 2},
    {"SongSetLength", 2},
    {"SongSetFileEnding", 2},
    {"SongSetAlbum", 2},
    {"SongAddTag", 2},
    {"SongRemoveTag", 2},

    {"AlbumAdd", 2},
    {"AlbumRemove", 1},
    {"AlbumSetName", 2},
    {"AlbumSetArtist", 2},
    {"AlbumAddTag", 2},
    {"AlbumRemoveTag", 2},

    {"ArtistAdd", 0},
    {"ArtistAddOrGet", 0},
    {"ArtistRemove", 1},
    {"ArtistSetName", 2},
    {"ArtistAddTag", 2},
    {"ArtistRemoveTag", 2},

    {"TagAdd", 2},
    {"TagRemove", 1},
    {"TagSetName", 2},
    {"TagSetParent", 2},
};

QString answerToString(const Moosick::LibraryChange change)
{
    const quint32 idx = (quint32) change.changeType;
    if (idx >= changeTypesAndParamCount.size())
        return QString::asprintf("Invalid(%d)", idx);

    const bool hasStr = Moosick::LibraryChange::hasStringArg(change.changeType);

    const QString name = changeTypesAndParamCount[idx].first;

    QString ret = name;
    ret += "(id=";
    ret += QString::number(change.subject);
    if (changeTypesAndParamCount[idx].second > 1) {
        if (hasStr)
            ret += QString(", name='") + change.name + "'";
        else {
            ret += QString(", op=") + QString::number(change.detail);
        }
    }
    ret += ")";

    if (Moosick::LibraryChange::isCreatingNewId(change.changeType)) {
        ret += " -> " + QString::number(change.detail);
    }

    return ret;
}

bool parseChange(const QString &str, Moosick::LibraryChange &change, QString &error)
{
    const QStringList parts = str.split("\t");
    if (parts.size() < 2) {
        error = QString("Not enough tab-separated parts (%1)").arg(parts.size());
        return false;
    }

    for (int i = 0; i < changeTypesAndParamCount.size(); ++i) {
        const auto tpParam = changeTypesAndParamCount[i];
        if (!tpParam.first.toLower().startsWith(parts[0].toLower()))
            continue;

        const bool correctNumberOfArgs = (parts.size() - 1 >= tpParam.second);

        if (!correctNumberOfArgs) {
            error = QString::asprintf("Wrong number of arguments. Got %d, expected %d.",
                                      parts.size() - 1,
                                      tpParam.second);
            return false;
        }

        const QString additionalArg = (parts.size() > 2) ? parts[2] : QString();
        bool subjectOk = true, detailOk = true;

        const Moosick::LibraryChange::Type tp = static_cast<Moosick::LibraryChange::Type>(i);
        const bool hasStringArg = Moosick::LibraryChange::hasStringArg(tp);

        quint32 subject, detail;
        QString name;

        if (tp == Moosick::LibraryChange::ArtistAdd || tp == Moosick::LibraryChange::ArtistAddOrGet) {
            name = parts[1];
        } else {
            subject = parts[1].toUInt(&subjectOk);
            detail = hasStringArg ? 0 : additionalArg.toUInt(&detailOk);
            if (hasStringArg)
                name = additionalArg;
        }

        if (!subjectOk) {
            error = QString("Subject not an ID: ") + parts[1];
            return false;
        }

        if (!detailOk) {
            error = QString("Detail not an ID: ") + parts[1];
            return false;
        }

        change.changeType = tp;
        change.subject = subject;
        change.detail = detail;
        change.name = name;

        return true;
    }

    error = QString("Not a recognized command: " + parts[0]);
    return false;
}

bool readLine(const QString &prompt, QString &line)
{
    QTextStream qtin(stdin);

    printf("%s", qPrintable(prompt));
    fflush(stdout);

    if (qtin.atEnd()) {
        printf("exit\n");
        return false;
    }

    line = qtin.readLine().trimmed();
    return true;
}

void printCommittedChanges(const QByteArray &data)
{
    QVector<Moosick::CommittedLibraryChange> appliedChanges;
    const QJsonArray json = parseJsonArray(data, "Committed Changes");
    if (json.isEmpty())
        qWarning().noquote() << data;


    if (!fromJson(json, appliedChanges)) {
        qWarning() << "Couldn't parse JSON result:";
        qWarning().noquote() << data;
    } else {
        for (const auto &ch : appliedChanges)
            qWarning().noquote() << ch.revision << ":" << answerToString(ch.change);
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("hostname", "Thy host");
    parser.addPositionalArgument("port", "Thy port");
    parser.addOption({{"timeout", "t"}, "How long to wait for a connection to host", "timeout", "1000"});
    parser.addOption({{"media", "m"}, "Media directory", "media", "."});
    parser.addOption({{"node", "n"}, "Node.js and Tools directory", "node", "."});
    parser.addOption({{"temp", "T"}, "Temp directory", "temp", "/tmp"});
    parser.process(app);

    const QStringList posArgs = parser.positionalArguments();
    if (posArgs.size() < 2)
        parser.showHelp();

    s_serverConfig.hostName = posArgs[0];
    s_serverConfig.port = posArgs[1].toUShort();
    s_serverConfig.timeout = parser.value("timeout").toInt();

    const QString mediaDir = parser.value("media");
    const QString toolsDir = parser.value("node");
    const QString tempDir = parser.value("temp");

    while (true) {
        QString line;
        if (!readLine(">>> ", line))
            return 0;

        ClientCommon::Message answer;

        if (line.isEmpty())
            continue;

        if (line.toLower() == "ping") {
            sendRecv(s_serverConfig, ClientCommon::Message{ ClientCommon::Ping }, answer);
            qWarning().noquote() << answer.format();
        }
        else if (line.toLower() == "id") {
            sendRecv(s_serverConfig, ClientCommon::Message{ ClientCommon::IdRequest }, answer);
            qWarning().noquote() << answer.format();
        }
        else if (QString("library").startsWith(line.toLower())) {
            sendRecv(s_serverConfig, ClientCommon::Message{ ClientCommon::LibraryRequest }, answer);
            Moosick::Library lib;
            const QJsonObject json = parseJsonObject(answer.data, "Library");
            if (!lib.deserializeFromJson(json)) {
                qWarning() << "Failed to de-serialize from JSON:";
                qWarning().noquote() << answer.data;
            }
            else {
                const QStringList libDump = lib.dumpToStringList();
                for (const QString &line : libDump)
                    qWarning().noquote() << line;
            }
        }
        else if (QString("bandcamp").startsWith(line.split("\t").first().toLower())) {
            const QStringList parts = line.split("\t").mid(1);
            if (parts.size() < 4) {
                qWarning() << "Usage: bandcamp    [url]    [artist name OR id]    [album name]    [revision]";
                continue;
            }

            const NetCommon::DownloadRequest download{
                NetCommon::DownloadRequest::BandcampAlbum,
                parts[0], parts[1].toUInt(), parts[1], parts[2], parts[3].toUInt()
            };

            const QVector<Moosick::CommittedLibraryChange> ret = ClientCommon::download(s_serverConfig, download, mediaDir, toolsDir, tempDir);
            for (const Moosick::CommittedLibraryChange &ch : ret)
                qWarning().noquote() << ch.revision << ":" << answerToString(ch.change);
        }
        else if (QString("youtube").startsWith(line.split("\t").first().toLower())) {
            const QStringList parts = line.split("\t").mid(1);
            if (parts.size() < 4) {
                qWarning() << "Usage: youtube    [url]    [artist name OR id]    [album name]    [revision]";
                continue;
            }

            const NetCommon::DownloadRequest download{
                NetCommon::DownloadRequest::YoutubeVideo,
                parts[0], parts[1].toUInt(), parts[1], parts[2], parts[3].toUInt()
            };

            const QVector<Moosick::CommittedLibraryChange> ret = ClientCommon::download(s_serverConfig, download, mediaDir, toolsDir, tempDir);
            for (const Moosick::CommittedLibraryChange &ch : ret)
                qWarning().noquote() << ch.revision << ":" << answerToString(ch.change);
        }
        else if (QString("history").startsWith(line.split("\t").first().toLower())) {
            const QStringList parts = line.split("\t").mid(1);
            if (parts.size() < 1) {
                qWarning() << "Needs revision number";
                continue;
            }
            sendRecv(s_serverConfig, ClientCommon::Message{ ClientCommon::ChangeListRequest, parts[0].toLocal8Bit()}, answer);
            printCommittedChanges(answer.data);
        }
        else if (QString("changes").startsWith(line.toLower())) {
            QVector<Moosick::LibraryChange> changes;

            while (true) {
                QString changeLine;
                if (!readLine("change >>> ", changeLine))
                    return 0;

                if (changeLine.isEmpty())
                    break;

                // read lines and parse them as LibraryChanges, stop as soon as we don't
                // have an \\ at the end
                const bool continues = changeLine.endsWith('\\');
                if (continues)
                    changeLine = changeLine.mid(0, changeLine.size() - 1).trimmed();

                // try to parse line
                Moosick::LibraryChange change;
                QString error;
                if (!parseChange(changeLine, change, error)) {
                    qWarning().noquote() << "Failed to parse Change:" << error;
                    continue;
                }

                changes << change;
                if (!continues)
                    break;
            }

            ClientCommon::Message message{ClientCommon::ChangesRequest};
            QDataStream out(&message.data, QIODevice::WriteOnly);
            out << changes;

            sendRecv(s_serverConfig, message, answer);
            printCommittedChanges(answer.data);
        }
        else {
            qWarning() << "Not a valid message type:" << line;
        }
    }

    return 0;
}
