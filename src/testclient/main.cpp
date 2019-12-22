#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDataStream>
#include <QTextStream>

#include <QTcpServer>
#include <QTcpSocket>

#include "library.hpp"
#include "messages.hpp"

static constexpr quint16 DEFAULT_PORT = 12345;

static QString s_hostname;
static QString s_port;
static int s_timeout;

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

    {"ArtistAdd", 1},
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
            ret += QString(", name='") + change.name;
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
        if (parts[0].toLower() != tpParam.first.toLower())
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

        if (tp == Moosick::LibraryChange::ArtistAdd) {
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

bool send(const NetCommon::Message &message, NetCommon::Message &answer)
{
    // try to connect to TCP server
    QTcpSocket sock;
    sock.connectToHost(s_hostname, s_port.toUShort());
    if (!sock.waitForConnected(s_timeout)) {
        qWarning() << "Unable to connect to" << s_hostname << ", port =" << s_port;
        return false;
    }

    NetCommon::send(&sock, message);
    NetCommon::receive(&sock, answer, s_timeout);

    return true;
}

bool sendChanges(QVector<Moosick::LibraryChange> &changes)
{
    // build message
    QByteArray data;
    QDataStream writer(&data, QIODevice::WriteOnly);
    writer << changes;
    NetCommon::Message msg{ NetCommon::ChangesRequest, data };

    // send
    NetCommon::Message answer;
    if (!send(msg, answer))
        return false;

    if (answer.tp == NetCommon::ChangesResponse) {
        QDataStream reader(answer.data);
        changes.clear();
        reader >> changes;
        return true;
    }

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

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("hostname", "Thy host");
    parser.addPositionalArgument("port", "Thy port");
    parser.addOption({{"timeout", "t"}, "How long to wait for a connection to host", "timeout", "1000"});
    parser.process(app);

    const QStringList posArgs = parser.positionalArguments();
    if (posArgs.size() < 2)
        parser.showHelp();

    s_hostname = posArgs[0];
    s_port = posArgs[1];
    s_timeout = parser.value("timeout").toInt();

    while (true) {
        QString line;
        if (!readLine(">>> ", line))
            return 0;

        NetCommon::Message answer;

        if (line.toLower() == "ping") {
            send(NetCommon::Message{ NetCommon::Ping }, answer);
            qWarning().noquote() << answer.format();
        }
        else if (QString("library").startsWith(line.toLower())) {
            send(NetCommon::Message{ NetCommon::LibraryRequest }, answer);
            Moosick::Library lib;
            lib.deserialize(answer.data);
            const QStringList libDump = lib.dumpToStringList();
            for (const QString &line : libDump)
                qWarning().noquote() << line;
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
                    changeLine = changeLine.mid(0, changeLine.size() - 1);

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

            NetCommon::Message message{NetCommon::ChangesRequest};
            QDataStream out(&message.data, QIODevice::WriteOnly);
            out << changes;

            send(message, answer);
            QDataStream in(answer.data);
            changes.clear();
            in >> changes;

            for (const Moosick::LibraryChange &ch : changes)
                qWarning().noquote() << answerToString(ch);
        }
        else {
            qWarning() << "Not a valid message type:" << line;
        }
    }

    return 0;
}
