#ifndef ROUTER_H
#define ROUTER_H

#include <QElapsedTimer>
#include <QObject>
#include <QUdpSocket>
#include <QtQmlIntegration>
#include "clientnode.h"
#include "message.h"

class ApplicationData;

class Router : public QObject
{
    Q_OBJECT
public:
    explicit Router(QObject *parent = nullptr);

    void setAppData(ApplicationData *appData);

    Q_PROPERTY(int listenPort READ listenPort CONSTANT)

    int listenPort() const { return 24400; }

signals:
    void messageReceived(Message message);

private slots:
    void readPendingDatagrams();
    void clientStateChanged(ClientNode::State state);

private:
    void receiveMessage(ClientNode::Connection connection, Message message);

    QUdpSocket *udp_socket = nullptr;

    /// List of clients in order of connecting.
    QList<ClientNode *> clients;

    ApplicationData *appData;
};

#endif // ROUTER_H
