#include "router.h"

#include <QDateTime>
#include <QHash>
#include <QMetaEnum>
#include <QNetworkDatagram>
#include "applicationdata.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include "networkdisplay.h"
#include <algorithm>

Router::Router(QObject *parent)
    : QObject{parent}
{
    udpSocket = new QUdpSocket(this);
    // FIXME: Should bind to this port exclusively (throw error if already used by another process)
    udpSocket->bind(QHostAddress::AnyIPv4, listenPort());
    connect(udpSocket, &QUdpSocket::readyRead, this, &Router::readPendingDatagrams);
}

void Router::setAppData(ApplicationData *appData)
{
    this->appData = appData;
}

QSet<ComponentId> Router::connectedComponents() const
{
    QSet<ComponentId> connected;
    for (const auto client : clients) {
        if (client->state() == ClientNode::State::Connected) {
            connected.insert(client->component);
        }
    }
    return connected;
}

void Router::sendMessage(Message message, ComponentId targetComponent, SystemId targetSystem)
{
    bool sent = false;
    for (const auto client : clients) {
        if (client->state() != ClientNode::State::TimedOut
            && (targetComponent == ComponentId::Broadcast || targetComponent == client->component)
            && (targetSystem == SystemId::Broadcast || targetSystem == client->system)) {
            client->sendMessage(message);
            sent = true;
        }
    }

    if (sent) {
        emit messageSent(message);
    }
}

void Router::sendMessageToTypes(Message message, const QSet<ComponentType> &targetTypes)
{
    bool sent = false;
    for (const auto client : clients) {
        if (client->state() == ClientNode::State::Connected
            && targetTypes.contains(client->type)) {
            client->sendMessage(message);
            sent = true;
        }
    }

    if (sent) {
        emit messageSent(message);
    }
}

void Router::readPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        auto datagram = udpSocket->receiveDatagram();
        for (qsizetype i = 0; i < datagram.data().size(); i++) { // iterators not recommended for QByteArray
            mavlink_message_t message_m;
            mavlink_status_t parser_status;
            if (mavlink_parse_char(MAVLINK_COMM_0,
                                   datagram.data().at(i),
                                   &message_m,
                                   &parser_status)) {
                receiveMessage(ClientNode::Connection{datagram.senderAddress(),
                                                      datagram.senderPort(),
                                                      udpSocket},
                               Message{Message::currentTime(), message_m});
            }
            // TODO: Handle repeated messages with CRC error to notify the user of a possible protocol definition mismatch
        }
    }
}

void Router::updateClientStates()
{
    if (updatingClientStates)
        return; // the outer call handles the state changes it caused
    updatingClientStates = true;

    const auto oldComponents = connectedComponents();

    const auto owners = clientOwners();
    bool anySuperseded = false;
    for (const auto client : std::as_const(clients)) {
        const auto state = client->state();
        if (state == ClientNode::State::Connected || state == ClientNode::State::Shadowed)
            client->setShadowed(owners.value({client->system, client->component}) != client);
        else if (state == ClientNode::State::TimedOut
                 && owners.contains({client->system, client->component}))
            anySuperseded = true;
    }

    updatingClientStates = false;

    // Removing a client here would delete its display while it's still emitting stateChanged
    if (anySuperseded)
        QMetaObject::invokeMethod(this, &Router::removeSupersededClients, Qt::QueuedConnection);

    const auto components = connectedComponents();
    if (components != oldComponents)
        emit connectedComponentsChanged(components);
}

void Router::removeSupersededClients()
{
    // Timed out clients with the ids now used by another client won't come back on their port
    const auto owners = clientOwners();
    QList<ClientNode *> superseded;
    for (const auto client : std::as_const(clients)) {
        if (client->state() == ClientNode::State::TimedOut
            && owners.contains({client->system, client->component})) {
            superseded.push_back(client);
        }
    }

    for (const auto client : std::as_const(superseded)) {
        qDebug().noquote() << "removing" << client->connection().toString()
                           << "superseded by a client with the same system and component id";
        clients.removeOne(client);
        emit clientRemoved(client);
        client->disconnect(); // in case it emits anything before deletion
        client->deleteLater();
    }
}

QHash<Router::SysComp, ClientNode *> Router::clientOwners() const
{
    // In order of connecting, the first active client owns the ids. A later client from the same
    // address takes over, as it is most likely a restarted node with a new port, and the previous
    // one would block it until timing out.
    QHash<SysComp, ClientNode *> owners;
    for (const auto client : std::as_const(clients)) {
        if (client->state() != ClientNode::State::Connected
            && client->state() != ClientNode::State::Shadowed)
            continue;

        const SysComp sysComp{client->system, client->component};
        const auto owner = owners.value(sysComp, nullptr);
        if (!owner || owner->connection().address == client->connection().address)
            owners.insert(sysComp, client);
    }
    return owners;
}

void Router::receiveMessage(ClientNode::Connection connection, Message message)
{
    const auto messageDebug = [&connection, &message]() {
        auto deb = qDebug().noquote(); // reuse the same debug stream to print to a single line
        const auto info = mavlink_get_message_info(&message.m);
        return deb << connection.toString()
                   << QString("%1:%2").arg(message.m.sysid).arg(message.m.compid) << info->name;
    };

    // get the connected client
    ClientNode *client = nullptr;
    {
        auto it = std::find_if(clients.cbegin(), clients.cend(), [&](const ClientNode *c) {
            return c->connection() == connection;
        });
        if (it != std::end(clients)) {
            client = *it;
        } else {
            // only register clients when receiving heartbeat
            const bool registered = message.id() == MessageId(MAVLINK_MSG_ID_HEARTBEAT);
            client = new ClientNode(this,
                                    connection,
                                    message.senderSystem(),
                                    message.senderComponent(),
                                    registered ? ClientNode::State::Connected
                                               : ClientNode::State::Unregistered);
            client->setAppData(appData);
            clients.push_back(client);
            connect(client, &ClientNode::stateChanged, this, &Router::updateClientStates);
            // decides whether this or another client with the same ids is shadowed
            updateClientStates();

            if (!registered)
                messageDebug() << "addding unregistered client, send HEARTBEAT for normal operation";
            else if (client->shadowed())
                messageDebug() << "another client for component" << message.m.compid
                               << "in system" << message.m.sysid;
            else
                messageDebug() << "registered a new client";

            emit clientAdded(client);
        }
    }
    Q_ASSERT(client);

    client->receiveMessage(message); // always process the message in that client
    if (client->state() != ClientNode::State::Connected)
        return; // allow only valid clients to affect any external state

    emit messageReceived(message);

    // pass the message to every subscriber, except those no longer sending heartbeats
    for (const auto listener : std::as_const(clients)) {
        if (listener->state() != ClientNode::State::TimedOut
            && listener->isSubscribed(message.id())) {
            listener->sendMessage(message);
        }
    }
}
