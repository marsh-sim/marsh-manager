#ifndef ROUTER_H
#define ROUTER_H

#include <QElapsedTimer>
#include <QObject>
#include <QUdpSocket>
#include "clientnode.h"
#include "message.h"
#include "networkdisplay.h"

class ApplicationData;

class Router : public QObject
{
    Q_OBJECT
public:
    explicit Router(QObject *parent = nullptr);

    void setAppData(ApplicationData *appData);

signals:

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
