#include "util.hpp"

#include <QTcpSocket>
#include <QProcess>

namespace ClientCommon {

bool sendChanges(
        const ServerConfig &config,
        const QVector<Moosick::LibraryChange> &changes,
        QVector<Moosick::LibraryChange> &answers)
{
    // try to connect to TCP server
    QTcpSocket sock;
    sock.connectToHost(config.hostName, config.port);
    if (!sock.waitForConnected(config.timeout)) {
        qWarning() << "Unable to connect to" << config.hostName << ", port =" << config.port;
        return false;
    }

    // build message
    QByteArray data;
    QDataStream writer(&data, QIODevice::WriteOnly);
    writer << changes;
    ClientCommon::Message msg{ ClientCommon::ChangesRequest, data };

    // send
    ClientCommon::Message answer;
    ClientCommon::send(&sock, msg);
    if (!ClientCommon::receive(&sock, answer, config.timeout))
        return false;

    if (answer.tp != ClientCommon::ChangesResponse)
        return false;

    QDataStream reader(answer.data);
    answers.clear();
    reader >> answers;

    return true;
}

int runProcess(
        const QString &program,
        const QStringList &args,
        QByteArray *out,
        QByteArray *err,
        int timeout)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(args);
    process.start();

    if (!process.waitForFinished(timeout)) {
        return -1;
    }

    if (out)
        *out = process.readAllStandardOutput();
    if (err)
        *err= process.readAllStandardError();
    return process.exitCode();
}

} //namespace ClientCommon
