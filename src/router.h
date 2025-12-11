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
    Q_PROPERTY(QSet<ComponentId> connectedComponents READ connectedComponents NOTIFY
                   connectedComponentsChanged FINAL)

    int listenPort() const { return 24400; }
    QSet<ComponentId> connectedComponents() const;

    void sendMessage(Message message,
                     ComponentId targetComponent = ComponentId::Broadcast,
                     SystemId targetSystem = SystemId::Broadcast);

    void sendMessageToTypes(Message message, const QSet<ComponentType> &targetTypes);

signals:
    void messageReceived(Message message);
    void messageSent(Message message);
    void clientAdded(ClientNode *client);
    void connectedComponentsChanged(QSet<ComponentId> components);

private slots:
    void readPendingDatagrams();
    void clientStateChanged(ClientNode::State state);

private:
    void receiveMessage(ClientNode::Connection connection, Message message);

    QUdpSocket *udpSocket = nullptr;

    /// List of clients in order of connecting.
    QList<ClientNode *> clients;

    ApplicationData *appData;
};

#endif // ROUTER_H
