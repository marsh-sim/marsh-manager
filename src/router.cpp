#include "router.h"

#include <QDateTime>
#include <QMetaEnum>
#include <QNetworkDatagram>
#include <algorithm>

Router::Router(QObject *parent)
    : QObject{parent}
    , start_timestamp{QDateTime::currentMSecsSinceEpoch() * 1000}
{
    udp_socket = new QUdpSocket(this);
    // FIXME: Should bind exclusively to this port (throw error if already used by another process)
    udp_socket->bind(QHostAddress::LocalHost, 24400);
    connect(udp_socket, &QUdpSocket::readyRead, this, &Router::readPendingDatagrams);

    running_timer.start();
}

void Router::readPendingDatagrams()
{
    while (udp_socket->hasPendingDatagrams()) {
        auto datagram = udp_socket->receiveDatagram();
        for (qsizetype i = 0; i < datagram.data().size(); i++) { // iterators not recommended for QByteArray
            mavlink_message_t message;
            mavlink_status_t parser_status;
            if (mavlink_parse_char(MAVLINK_COMM_0, datagram.data().at(i), &message, &parser_status)) {
                receiveMessage(ClientNode::Connection{datagram.senderAddress(),
                                                      datagram.senderPort()},
                               currentTime(),
                               message);
            }
            // TODO: Handle repeated messages with CRC error to notify the user of a possible protocol definition mismatch
        }
    }
}

void Router::receiveMessage(ClientNode::Connection connection,
                            qint64 timestamp,
                            mavlink_message_t message)
{
    const auto info = mavlink_get_message_info(&message);
    auto deb = qDebug().noquote(); // reuse the same debug stream to print to a single line
    deb << QString("%1:%2").arg(connection.address.toString()).arg(connection.port)
        << QString("%1:%2").arg(message.sysid).arg(message.compid) << info->name;

    // get the connected client
    ClientNode *client = nullptr;
    {
        auto it = std::find_if(clients.cbegin(), clients.cend(), [&](const ClientNode *c) {
            return c->connection() == connection;
        });
        if (it != std::end(clients)) {
            client = *it;
        } else if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            // only register clients when receiving heartbeat
            client = new ClientNode(this, connection);
            client->system_id = message.sysid;
            client->component_id = message.compid;
            clients.push_back(client);
            deb << "registered a new client";
        }
    }
    if (!client) {
        deb << "message from unregistered client, send HEARTBEAT first";
        return;
    }

    client->receiveMessage(timestamp, message);

    QByteArray send_buffer(MAVLINK_MAX_PACKET_LEN, Qt::Initialization::Uninitialized);
    for (const auto listener : clients) {
        if (listener->subscribed_messages.contains(message.msgid)) {
            message.seq = listener->message_sequence_number++;
            const auto length = mavlink_msg_to_send_buffer((quint8 *) send_buffer.data(), &message);
            udp_socket->writeDatagram(send_buffer,
                                      length,
                                      listener->connection().address,
                                      listener->connection().port);
        }
    }
}

qint64 Router::currentTime()
{
    return start_timestamp + running_timer.nsecsElapsed() / 1000;
}
