#ifndef ROUTER_H
#define ROUTER_H

#include <QElapsedTimer>
#include <QHash>
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
    const QList<ClientNode *> &getClients() const { return clients; }

    void sendMessage(Message message,
                     ComponentId targetComponent = ComponentId::Broadcast,
                     SystemId targetSystem = SystemId::Broadcast);

    void sendMessageToTypes(Message message, const QSet<ComponentType> &targetTypes);

signals:
    void messageReceived(const Message &message);
    void messageSent(const Message &message);
    void clientAdded(ClientNode *client);
    /// Emitted before the client is deleted
    void clientRemoved(ClientNode *client);
    void connectedComponentsChanged(QSet<ComponentId> components);

private slots:
    void readPendingDatagrams();
    void updateClientStates();
    void removeSupersededClients();

private:
    using SysComp = QPair<SystemId, ComponentId>;

    void receiveMessage(ClientNode::Connection connection, Message message);
    /// Client owning each system and component id pair, the others with same ids are shadowed
    QHash<SysComp, ClientNode *> clientOwners() const;

    QUdpSocket *udpSocket = nullptr;

    /// List of clients in order of connecting.
    QList<ClientNode *> clients;
    /// Guards against recursion, as updating client states emits their stateChanged
    bool updatingClientStates = false;

    ApplicationData *appData;
};

#endif // ROUTER_H
