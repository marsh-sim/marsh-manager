#ifndef ROUTER_H
#define ROUTER_H

#include <QObject>
#include <QUdpSocket>
#include "mavlink/minimal/mavlink.h"

class Router : public QObject
{
    Q_OBJECT
public:
    explicit Router(QObject *parent = nullptr);

    struct Client {
        QHostAddress address;
        int port;
    };

signals:

private slots:
    void readPendingDatagrams();

private:
    void messageReceived(Client client, qint64 timestamp, mavlink_message_t message);

    QUdpSocket* udp_socket = nullptr;
};

#endif // ROUTER_H
