#include "signalhandler.hpp"

#include <QSocketNotifier>
#include <QDebug>

#include <sys/types.h>
#include <sys/socket.h>
#include <csignal>

#include <unistd.h>

int SignalHandler::s_sigFd[2] = {0, 0};

SignalHandler::SignalHandler()
    : QObject(nullptr)
{
    socketpair(AF_UNIX, static_cast<qint32>(SOCK_STREAM), 0, s_sigFd);
    m_sigNotifier.reset(new QSocketNotifier(s_sigFd[1], QSocketNotifier::Read, this));
    connect(m_sigNotifier.data(), &QSocketNotifier::activated, this, &SignalHandler::handler);

    // install sigaction
    struct sigaction action;
    (void)memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SignalHandler::signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGINT, &action, nullptr);
}

SignalHandler::~SignalHandler()
{
}

void SignalHandler::signalHandler(int)
{
    char a = 1;
    write(s_sigFd[0], &a, 1);
}

void SignalHandler::handler()
{
    m_sigNotifier->setEnabled(false);

    char tmp;
    read(s_sigFd[1], &tmp, 1);

    emit signalled();

    m_sigNotifier->setEnabled(true);
}
