#ifndef CLIENTNODE_H
#define CLIENTNODE_H

#include <QHostAddress>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include "frequencyestimator.h"
#include "message.h"

class ApplicationData;

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
        /// No HEARTBEAT has been received from this connection, only different messages
        Unregistered,
    };
    Q_ENUM(State)

    /// Custom mode corresponding to MARSH_MODE_FLAGS
    enum class CustomMode {
        /// Operating as normal
        None,
        /// Only interested in receiving a single message
        SingleMessage,
        /// Interested in receiving all messages
        AllMessages,
    };
    Q_ENUM(CustomMode)

    struct Connection
    {
        QHostAddress address;
        int port;
        QUdpSocket *udpSocket;

        bool operator==(const Connection &other) const
        {
            return address == other.address && port == other.port;
        }

        QString toString();
    };
    SystemId system;
    ComponentId component;

    explicit ClientNode(QObject *parent,
                        Connection connection,
                        SystemId system,
                        ComponentId component,
                        State state = State::Connected);
    void setAppData(ApplicationData *appData);

    Q_PROPERTY(Connection connection READ connection CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool shadowed READ shadowed WRITE setShadowed NOTIFY shadowedChanged FINAL)
    Q_PROPERTY(QSet<MessageId> subscribedMessages READ subscribedMessages NOTIFY
                   subscribedMessagesChanged FINAL)
    Q_PROPERTY(CustomMode customMode READ customMode NOTIFY subscribedMessagesChanged FINAL)

    Connection connection() const { return _connection; }
    State state() const { return _state; }
    bool shadowed() const { return _state == State::Shadowed; }
    /// Messages that this client is interested in receiving
    QSet<MessageId> subscribedMessages() const;
    CustomMode customMode() const { return _customMode; }
    std::optional<MessageId> singleMessage() const;

    void setShadowed(bool shadowed);

    struct MessageHistory
    {
        Message last;
        FrequencyEstimator frequency;
    };

    FrequencyEstimator receiveFrequency{};
    FrequencyEstimator sendFrequency{};
    QMap<MessageId, MessageHistory> receivedMessages;
    QMap<MessageId, MessageHistory> sentMessages;
    /// Messages with sending rate limited, minimal interval in microseconds
    QMap<MessageId, qint64> messageLimitIntervals;
    quint8 sendingSequenceNumber = 0;

    /// Messages always subscribed to based on component id
    static const QMap<ComponentId, QSet<MessageId>> componentSubscriptions;

signals:
    void stateChanged(State state);
    void shadowedChanged(bool shadowed);
    void messageReceived(Message message);
    void messageSent(Message message);
    void subscribedMessagesChanged(QSet<MessageId> ids);

private slots:
    void heartbeatTimerElapsed(void);

private:
    friend class Router;
    // All messages are supposed to be sent through Router
    void receiveMessage(Message message);
    void sendMessage(Message message);

    void handleCommand(Message message);

    ApplicationData* appData;
    Connection _connection;
    State _state;
    /// This is the first *currently* connected client with this system id and component id
    bool firstSysidCompid;
    CustomMode _customMode;
    MessageId customModeMessage;
    QSet<MessageId> _subscribedMessages;

    QTimer *heartbeatTimer = nullptr;
};

#endif // CLIENTNODE_H
