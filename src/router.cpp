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
    udp_socket = new QUdpSocket(this);
    // FIXME: Should bind exclusively to this port (throw error if already used by another process)
    udp_socket->bind(QHostAddress::LocalHost, listenPort());
    connect(udp_socket, &QUdpSocket::readyRead, this, &Router::readPendingDatagrams);
}

void Router::setAppData(ApplicationData *appData)
{
    this->appData = appData;
}

void Router::readPendingDatagrams()
{
    while (udp_socket->hasPendingDatagrams()) {
        auto datagram = udp_socket->receiveDatagram();
        for (qsizetype i = 0; i < datagram.data().size(); i++) { // iterators not recommended for QByteArray
            mavlink_message_t message_m;
            mavlink_status_t parser_status;
            if (mavlink_parse_char(MAVLINK_COMM_0,
                                   datagram.data().at(i),
                                   &message_m,
                                   &parser_status)) {
                receiveMessage(ClientNode::Connection{datagram.senderAddress(),
                                                      datagram.senderPort()},
                               Message{Message::currentTime(), message_m});
            }
            // TODO: Handle repeated messages with CRC error to notify the user of a possible protocol definition mismatch
        }
    }
}

void Router::clientStateChanged(ClientNode::State state)
{
    if (state == ClientNode::State::TimedOut) {
        // some client previously shadowed may be the first component now

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
    }
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
        } else if (message.m.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            // only register clients when receiving heartbeat

            // look for other clients with the same system and component id
            it = std::find_if(clients.cbegin(), clients.cend(), [&](const ClientNode *c) {
                return c->system == message.senderSystem()
                       && c->component == message.senderComponent();
            });
            bool syscomp_free = it == std::end(clients);

            client = new ClientNode(this,
                                    connection,
                                    message.senderSystem(),
                                    message.senderComponent(),
                                    syscomp_free ? ClientNode::State::Connected
                                                 : ClientNode::State::Shadowed);
            clients.push_back(client);
            connect(client, &ClientNode::stateChanged, this, &Router::clientStateChanged);
            appData->networkDisplay()->addClient(client);

            if (syscomp_free)
                messageDebug() << "registered a new client";
            else
                messageDebug() << "another client for component" << message.m.compid << "in system"
                               << message.m.sysid;
        }
    }
    if (!client) {
        messageDebug() << "message from unregistered client, send HEARTBEAT first";
        return;
    }

    client->receiveMessage(message); // always process the message in that client
    if (client->state() != ClientNode::State::Connected)
        return; // allow only valid clients to affect any external state

    emit messageReceived(message);

    // broadcast the message to every subscriber
    QByteArray send_buffer(MAVLINK_MAX_PACKET_LEN, Qt::Initialization::Uninitialized);
    for (const auto listener : clients) {
        if (listener->subscribed_messages.contains(message.id())) {
            message.m.seq = listener->sending_sequence_number++;
            const auto length = mavlink_msg_to_send_buffer((quint8 *) send_buffer.data(),
                                                           &message.m);
            udp_socket->writeDatagram(send_buffer,
                                      length,
                                      listener->connection().address,
                                      listener->connection().port);
        }
    }
}
