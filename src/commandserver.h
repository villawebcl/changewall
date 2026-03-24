#pragma once

#include <QObject>
#include <QStringList>

class QLocalServer;
class QLocalSocket;

class CommandServer : public QObject {
    Q_OBJECT

public:
    explicit CommandServer(QObject *parent = nullptr);

    bool listen(const QString &serverName, QString *errorMessage = nullptr);
    static bool sendCommand(const QString &serverName, const QStringList &arguments, QString *errorMessage = nullptr);

signals:
    void commandReceived(const QStringList &arguments);

private:
    void handleConnection(QLocalSocket *socket);

    QLocalServer *m_server = nullptr;
};
