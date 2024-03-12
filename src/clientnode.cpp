#include "clientnode.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect

ClientNode::ClientNode(
    QObject *parent, Connection connection, SystemId system, ComponentId component, State state)
    : QObject{parent}
    , _connection{connection}
    , system{system}
    , component{component}
    , _state{state}
{
    firstSysidCompid = state == State::Shadowed;

    heartbeatTimer = new QTimer(this);
    heartbeatTimer->setSingleShot(true);
    // expect at least 1 Hz and allow max five messages to be lost
    heartbeatTimer->setInterval(5000);
    heartbeatTimer->start();
    connect(heartbeatTimer, &QTimer::timeout, this, &ClientNode::heartbeatTimerElapsed);
}

void ClientNode::setShadowed(bool shadowed)
{
    firstSysidCompid = shadowed;
    if (_state == State::Connected || _state == State::Shadowed) {
        _state = firstSysidCompid ? State::Shadowed : State::Connected;
        emit stateChanged(_state);
    }
    emit shadowedChanged(this->shadowed());
}

void ClientNode::receiveMessage(Message message)
{
    last_published_message[message.id()] = message;

    if (message.id() == MessageId(MAVLINK_MSG_ID_HEARTBEAT)) {
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&message.m, &heartbeat);

        heartbeatTimer->start();
    }

    emit messageReceived(message);
}

void ClientNode::heartbeatTimerElapsed()
{
    _state = State::TimedOut;
    emit stateChanged(_state);
}

QString ClientNode::Connection::toString()
{
    return QString("%1:%2").arg(address.toString()).arg(port);
}
