#pragma once

#include <QObject>

class QSocketNotifier;

class SignalHandler : public QObject
{
    Q_OBJECT

public:
    SignalHandler();
    ~SignalHandler();

signals:
    void signalled();

private Q_SLOTS:
    void handler();

private:
    static void signalHandler(int);
    static int s_sigFd[2];

    QScopedPointer<QSocketNotifier> m_sigNotifier;
};
