#ifndef ROUTER_H
#define ROUTER_H

#include <QElapsedTimer>
#include <QObject>
#include <QUdpSocket>
#include "mavlink/minimal/mavlink.h"
#include "src/clientnode.h"

class Router : public QObject
{
    Q_OBJECT
public:
    explicit Router(QObject *parent = nullptr);

signals:

private slots:
    void readPendingDatagrams();

private:
    void receiveMessage(ClientNode::Connection connection,
                        qint64 timestamp,
                        mavlink_message_t message);
    /// Timestamp in microseconds since epoch, as required by the "tlog" files
    qint64 currentTime();

    QUdpSocket *udp_socket = nullptr;

    /// Timestamp of object creation in microseconds from epoch
    const qint64 start_timestamp;
    /// Timer started on object creation to get better accuracy than system clock
    QElapsedTimer running_timer;

    QList<ClientNode *> clients;
};

#endif // ROUTER_H
