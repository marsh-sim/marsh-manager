#include "router.h"

#include <QNetworkDatagram>
#include <QTime>


Router::Router(QObject *parent)
    : QObject{parent}
{
    udp_socket = new QUdpSocket(this);
    udp_socket->bind(QHostAddress::LocalHost, 24400);

    connect(udp_socket, &QUdpSocket::readyRead, this, &Router::readPendingDatagrams);
}

void Router::readPendingDatagrams()
{
    while (udp_socket->hasPendingDatagrams()) {
        auto datagram = udp_socket->receiveDatagram();
        for (qsizetype i = 0; i < datagram.data().size(); i++) { // iterators not recommended for QByteArray
            mavlink::mavlink_message_t c_message;
            mavlink::mavlink_status_t parser_status;
            if(mavlink::mavlink_parse_char(0, datagram.data().at(i), &c_message, &parser_status)) {
                auto now = QDateTime::currentDateTime();
                qint64 timestamp = now.toMSecsSinceEpoch() * 1000; // TODO: Handle proper microsecond accuracy
                messageReceived(Client{ datagram.senderAddress(), datagram.senderPort() }, timestamp, c_message);
            }
        }
    }
}

void Router::messageReceived(Client client, qint64 timestamp, mavlink::mavlink_message_t c_message)
{
    auto map = mavlink::MsgMap{c_message};
    if (c_message.msgid == mavlink::minimal::msg::HEARTBEAT::MSG_ID) {
        mavlink::minimal::msg::HEARTBEAT heartbeat{};
        heartbeat.deserialize(map);
        qDebug() << heartbeat.NAME << "from component" << c_message.compid;
    }
}
