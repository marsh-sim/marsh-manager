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

    autoSubscribe();
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
    lastReceivedMessage[message.id()] = message;

    if (message.id() == MessageId(MAVLINK_MSG_ID_HEARTBEAT)) {
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&message.m, &heartbeat);

        heartbeatTimer->start();

        if (_state == State::Unregistered) {
            _state = firstSysidCompid ? State::Shadowed : State::Connected;
            emit stateChanged(_state);
        }

        // TODO: Define in XML with description:
        // Request Manager to only send one specific message, for resource limited nodes.
        // The requested message id should be sent in the lowest three bytes.
        const quint32 MARSH_MODE_SINGLE_MESSAGE = 0x0100'0000;
        if (heartbeat.custom_mode & MARSH_MODE_SINGLE_MESSAGE) {
            singleMessage = MessageId(heartbeat.custom_mode & 0x00FF'FFFF);
        } else {
            singleMessage = {};
        }
    }

    emit messageReceived(message);
}

void ClientNode::sendMessage(Message message)
{
    if (singleMessage && message.id() != *singleMessage) {
        return;
    }

    QByteArray send_buffer(MAVLINK_MAX_PACKET_LEN, Qt::Initialization::Uninitialized);
    // message.m.seq = sendingSequenceNumber++; // FIXME: needs to be repacked afterwards, otherwise breaks CRC
    const auto length = mavlink_msg_to_send_buffer((quint8 *) send_buffer.data(), &message.m);
    auto sent = _connection.udpSocket->writeDatagram(send_buffer,
                                                     length,
                                                     _connection.address,
                                                     _connection.port);
    if (sent < 0) {
        qDebug() << "Error sending to" << _connection.port;
    }

    lastSentMessage[message.id()] = message;
    emit messageSent(message);
}

void ClientNode::heartbeatTimerElapsed()
{
    if (_state != State::Unregistered) {
        _state = State::TimedOut;
        emit stateChanged(_state);
    }
}

void ClientNode::autoSubscribe()
{
    switch (component.value()) {
    case MARSH_COMP_ID_FLIGHT_MODEL:
        subscribedMessages << MessageId(MAVLINK_MSG_ID_MANUAL_CONTROL);
        break;
    case MARSH_COMP_ID_INSTRUMENTS:
        subscribedMessages << MessageId(MAVLINK_MSG_ID_MANUAL_CONTROL);
        subscribedMessages << MessageId(MAVLINK_MSG_ID_MANUAL_SETPOINT);
        subscribedMessages << MessageId(MAVLINK_MSG_ID_SIM_STATE);
        break;
    case MARSH_COMP_ID_VISUALISATION:
    case MARSH_COMP_ID_MOTION_PLATFORM:
    case MARSH_COMP_ID_GSEAT:
        subscribedMessages << MessageId(MAVLINK_MSG_ID_SIM_STATE);
        break;
    }
}

QString ClientNode::Connection::toString()
{
    return QString("%1:%2").arg(address.toString()).arg(port);
}
