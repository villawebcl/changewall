#include "commandserver.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QTextStream>

CommandServer::CommandServer(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, [this]() {
        while (m_server->hasPendingConnections()) {
            handleConnection(m_server->nextPendingConnection());
        }
    });
}

bool CommandServer::listen(const QString &serverName, QString *errorMessage)
{
    QLocalServer::removeServer(serverName);
    if (m_server->listen(serverName)) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = m_server->errorString();
    }
    return false;
}

bool CommandServer::sendCommand(const QString &serverName, const QStringList &arguments, QString *errorMessage)
{
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (!socket.waitForConnected(1000)) {
        if (errorMessage) {
            *errorMessage = socket.errorString();
        }
        return false;
    }

    socket.write(arguments.join(QLatin1Char('\n')).toUtf8());
    socket.flush();
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    return true;
}

void CommandServer::handleConnection(QLocalSocket *socket)
{
    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        const QString payload = QString::fromUtf8(socket->readAll());
        const QStringList arguments = payload.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        if (!arguments.isEmpty()) {
            emit commandReceived(arguments);
        }
        socket->disconnectFromServer();
        socket->deleteLater();
    });
}
