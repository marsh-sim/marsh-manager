#include "router.h"

#include <QDateTime>
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

void Router::clientStateChanged(ClientNode::State state)
{
    const auto oldComponents = connectedComponents();

    using SysComp = QPair<SystemId, ComponentId>;
    QSet<SysComp> connected_sys_comp{};

    for (auto client : clients) { // iterate in order of connecting
        if (client->state() == ClientNode::State::TimedOut)
            continue;

        SysComp sys_comp{client->system, client->component};
        if (!connected_sys_comp.contains(sys_comp)) {
            client->setShadowed(false);
            connected_sys_comp.insert(sys_comp);
        } else {
            client->setShadowed(true);
        }
    }

    const auto components = connectedComponents();
    if (components != oldComponents)
        emit connectedComponentsChanged(components);
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
            ClientNode::State state = ClientNode::State::Unregistered;
            // only register clients when receiving heartbeat
            if (message.id() == MessageId(MAVLINK_MSG_ID_HEARTBEAT)) {
                // look for already connected clients with the same system and component id
                it = std::find_if(clients.cbegin(), clients.cend(), [&](const ClientNode *c) {
                    return c->system == message.senderSystem()
                           && c->component == message.senderComponent()
                           && c->state() == ClientNode::State::Connected;
                });
                bool syscomp_free = it == std::end(clients);
                state = syscomp_free ? ClientNode::State::Connected : ClientNode::State::Shadowed;

                if (syscomp_free) {
                    emit connectedComponentsChanged(connectedComponents());
                    messageDebug() << "registered a new client";
                } else
                    messageDebug() << "another client for component" << message.m.compid
                                   << "in system" << message.m.sysid;
            } else {
                messageDebug()
                    << "addding unregistered client, send HEARTBEAT for normal operation";
            }

            client = new ClientNode(this,
                                    connection,
                                    message.senderSystem(),
                                    message.senderComponent(),
                                    state);
            client->setAppData(appData);
            clients.push_back(client);
            connect(client, &ClientNode::stateChanged, this, &Router::clientStateChanged);
            emit clientAdded(client);
        }
    }
    Q_ASSERT(client);

    client->receiveMessage(message); // always process the message in that client
    if (client->state() != ClientNode::State::Connected)
        return; // allow only valid clients to affect any external state

    emit messageReceived(message);

    // pass the message to every subscriber
    for (const auto listener : clients) {
        if (listener->customMode() == ClientNode::CustomMode::AllMessages
            || listener->subscribedMessages().contains(message.id())) {
            listener->sendMessage(message);
        }
    }
}
