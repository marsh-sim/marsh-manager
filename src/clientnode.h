#ifndef CLIENTNODE_H
#define CLIENTNODE_H

#include <QHostAddress>
#include <QObject>
#include "mavlink/minimal/mavlink.h"

class ClientNode : public QObject
{
    Q_OBJECT
public:
    struct Connection
    {
        QHostAddress address;
        int port;

        bool operator==(const Connection &other) const
        {
            return address == other.address && port == other.port;
        }
    };
    quint8 system_id;
    quint8 component_id;

    struct TimedMessage
    {
        qint64 timestamp;
        mavlink_message_t message;
    };

    explicit ClientNode(QObject *parent, Connection connection);

    Q_PROPERTY(Connection connection READ connection CONSTANT)
    Connection connection() const { return _connection; };

    void receiveMessage(qint64 timestamp, mavlink_message_t message);

    QMap<quint32, TimedMessage> last_published_message;
    QList<quint32> subscribed_messages;
    quint8 message_sequence_number = 0;
signals:

private:
    Connection _connection;
};

#endif // CLIENTNODE_H
