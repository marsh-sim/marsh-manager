#ifndef CLIENTNODE_H
#define CLIENTNODE_H

#include <QHostAddress>
#include <QObject>
#include <QTimer>
#include "message.h"

class ClientNode : public QObject
{
    Q_OBJECT
public:
    enum class State {
        /// Client is connected and working normally
        Connected,
        /// A previously connected client has same system and component id, can only receive messages
        Shadowed,
        /// Client has not sent a HEARTBEAT in the required time window
        TimedOut,
    };
    Q_ENUM(State)

    struct Connection
    {
        QHostAddress address;
        int port;

        bool operator==(const Connection &other) const
        {
            return address == other.address && port == other.port;
        }
    };
    SystemId system;
    ComponentId component;

    explicit ClientNode(QObject *parent,
                        Connection connection,
                        SystemId system,
                        ComponentId component,
                        State state = State::Connected);

    Q_PROPERTY(Connection connection READ connection CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool shadowed READ shadowed WRITE setShadowed NOTIFY shadowedChanged FINAL)

    Connection connection() const { return _connection; }
    State state() const { return _state; }
    bool shadowed() const { return _state == State::Shadowed; }

    void setShadowed(bool shadowed);

    void receiveMessage(Message message);

    QMap<MessageId, Message> last_published_message;
    QSet<MessageId> subscribed_messages;
    quint8 sending_sequence_number = 0;
signals:
    void stateChanged(State state);
    void shadowedChanged(bool shadowed);

private slots:
    void heartbeatTimerElapsed(void);

private:
    Connection _connection;
    State _state;
    /// This is the first *currently* connected client with this system id and component id
    bool first_sysid_compid;

    QTimer *heartbeat_timer = nullptr;
};

#endif // CLIENTNODE_H
