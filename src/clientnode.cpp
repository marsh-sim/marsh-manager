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
    first_sysid_compid = state == State::Shadowed;

    heartbeat_timer = new QTimer(this);
    heartbeat_timer->setSingleShot(true);
    // expect at least 1 Hz and allow max five messages to be lost
    heartbeat_timer->setInterval(5000);
    heartbeat_timer->start();
    connect(heartbeat_timer, &QTimer::timeout, this, &ClientNode::heartbeatTimerElapsed);
}

void ClientNode::setShadowed(bool shadowed)
{
    first_sysid_compid = shadowed;
    if (_state == State::Connected || _state == State::Shadowed) {
        _state = first_sysid_compid ? State::Shadowed : State::Connected;
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

        heartbeat_timer->start();
    }
}

void ClientNode::heartbeatTimerElapsed()
{
    _state = State::TimedOut;
    emit stateChanged(_state);
}
