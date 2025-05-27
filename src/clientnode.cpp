#include "clientnode.h"
#include "applicationdata.h"
#include "dialectinfo.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include <variant>

ClientNode::ClientNode(
    QObject *parent, Connection connection, SystemId system, ComponentId component, State state)
    : QObject{parent}
    , _connection{connection}
    , system{system}
    , component{component}
    , type{MAV_TYPE_GENERIC}
    , _state{state}
    , customModeMessage(0)
{
    firstSysidCompid = state == State::Shadowed;

    heartbeatTimer = new QTimer(this);
    heartbeatTimer->setSingleShot(true);
    // expect at least 1 Hz and allow max five messages to be lost
    heartbeatTimer->setInterval(5000);
    heartbeatTimer->start();
    connect(heartbeatTimer, &QTimer::timeout, this, &ClientNode::heartbeatTimerElapsed);
}

void ClientNode::setAppData(ApplicationData *appData)
{
    this->appData = appData;
}

QSet<MessageId> ClientNode::subscribedMessages() const
{
    if (_customMode == CustomMode::SingleMessage)
        return {customModeMessage};
    else if (_customMode == CustomMode::AllMessages)
        return {};
    else
        return _subscribedMessages;
}

void ClientNode::setShadowed(bool shadowed)
{
    if (shadowed != firstSysidCompid) {
        firstSysidCompid = shadowed;
        if (_state == State::Connected || _state == State::Shadowed) {
            _state = firstSysidCompid ? State::Shadowed : State::Connected;
            emit stateChanged(_state);
        }
        emit shadowedChanged(this->shadowed());
    }
}

void ClientNode::receiveMessage(Message message)
{
    receiveFrequency.addTime(message.timestamp);
    if (!receivedMessages.contains(message.id())) {
        receivedMessages[message.id()] = MessageHistory{message, FrequencyEstimator{}};
    }
    receivedMessages[message.id()].last = message;
    receivedMessages[message.id()].frequency.addTime(message.timestamp);

    if (message.id() == MessageId(MAVLINK_MSG_ID_HEARTBEAT)) {
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&message.m, &heartbeat);

        heartbeatTimer->start();

        if (type != ComponentType(heartbeat.type)) {
            type = ComponentType(heartbeat.type);
            // Automatically subscribe to all messages based on component type
            if (componentSubscriptions.contains(type)) {
                _subscribedMessages.unite(componentSubscriptions[type]);
                emit subscribedMessagesChanged(subscribedMessages());
            }
        }

        if (_state == State::Unregistered || _state == State::TimedOut) {
            _state = firstSysidCompid ? State::Shadowed : State::Connected;
            emit stateChanged(_state);
        }

        // Request Manager to only send one specific message, for resource limited nodes.
        // The requested message id should be sent in the lowest three bytes.
        const auto old_message = customModeMessage;
        const auto old_mode = _customMode;
        customModeMessage = MessageId(heartbeat.custom_mode & 0x00FF'FFFF);
        const quint32 mode_bits = 0x0300'0000;
        if ((heartbeat.custom_mode & mode_bits) == MARSH_MODE_SINGLE_MESSAGE) {
            _customMode = CustomMode::SingleMessage;
        } else if ((heartbeat.custom_mode & mode_bits) == MARSH_MODE_ALL_MESSAGES) {
            _customMode = CustomMode::AllMessages;
        } else if ((heartbeat.custom_mode & mode_bits) == 0) {
            _customMode = CustomMode::None;
        } else {
            qCritical() << "Mutually exclusive bits set in heartbeat.custom_mode received from"
                        << _connection.toString();
        }
        if (customModeMessage != old_message || _customMode != old_mode)
            emit subscribedMessagesChanged(subscribedMessages());
    } else if (message.id() == MessageId(MAVLINK_MSG_ID_COMMAND_INT) || message.id() == MessageId(MAVLINK_MSG_ID_COMMAND_LONG)) {
        handleCommand(message);
    }

    emit messageReceived(message);
}

void ClientNode::sendMessage(Message message)
{
    if (messageLimitIntervals.contains(message.id())) {
        const auto lastTime = sentMessages[message.id()].last.timestamp;
        if (message.timestamp - lastTime < messageLimitIntervals[message.id()]) {
            return; // too soon to send this one
        }
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

    const auto sendTime = Message::currentTime(); // take the time now to include in-app delays
    sendFrequency.addTime(sendTime);
    if (!sentMessages.contains(message.id())) {
        sentMessages[message.id()] = MessageHistory{message, FrequencyEstimator{}};
    }
    sentMessages[message.id()].last = message;
    sentMessages[message.id()].frequency.addTime(sendTime);

    emit messageSent(message);
}

void ClientNode::handleCommand(Message message)
{
    SystemId targetSystem = SystemId::Broadcast;
    ComponentId targetComponent = ComponentId::Broadcast;
    std::optional<uint8_t> frame;
    uint16_t command;
    float param1, param2, param3, param4, param7;
    std::variant<int32_t, float> param5, param6;

    if (message.id() == MessageId(MAVLINK_MSG_ID_COMMAND_INT)) {
        mavlink_command_int_t command_int;
        mavlink_msg_command_int_decode(&message.m, &command_int);
        targetSystem = SystemId(command_int.target_system);
        targetComponent = ComponentId(command_int.target_component);
        frame = command_int.frame;
        command = command_int.command;
        param1 = command_int.param1;
        param2 = command_int.param2;
        param3 = command_int.param3;
        param4 = command_int.param4;
        param5 = command_int.x;
        param6 = command_int.y;
        param7 = command_int.z;
    } else if (message.id() == MessageId(MAVLINK_MSG_ID_COMMAND_LONG)) {
        mavlink_command_long_t command_long;
        mavlink_msg_command_long_decode(&message.m, &command_long);
        targetSystem = SystemId(command_long.target_system);
        targetComponent = ComponentId(command_long.target_component);
        command = command_long.command;
        param1 = command_long.param1;
        param2 = command_long.param2;
        param3 = command_long.param3;
        param4 = command_long.param4;
        param5 = command_long.param5;
        param6 = command_long.param6;
        param7 = command_long.param7;
    } else {
        qWarning() << "Incorrect message passed to handleCommand:" << message.id().toString();
        return;
    }

    if (targetSystem != system || (targetComponent != component && targetComponent != ComponentId(appData->localComponentId()))) {
        return;
    }

    uint8_t result = MAV_RESULT_UNSUPPORTED; // assume command not recognized
    if (command == MAV_CMD_SET_MESSAGE_INTERVAL) {
        result = MAV_RESULT_DENIED; // assume parameters not accepted
        qint32 id_num = qRound(param1);
        qint64 interval = qRound(param2);
        int target = qRound(param7);
        if (id_num >= 0 && id_num <= 16777215 && target == 1) {
            // only allow subscribing to valid message id and for the client itself

            qsizetype old_count = _subscribedMessages.count();
            MessageId id{static_cast<uint32_t>(id_num)};
            if (interval == -1) {
                _subscribedMessages.remove(id);
                messageLimitIntervals.remove(id);
                result = MAV_RESULT_ACCEPTED;
            } else if (interval == 0) {
                _subscribedMessages.insert(id);
                messageLimitIntervals.remove(id);
                result = MAV_RESULT_ACCEPTED;
            } else if (interval > 0) {
                _subscribedMessages.insert(id);
                messageLimitIntervals.insert(id, interval);
                result = MAV_RESULT_ACCEPTED;
            }

            if (_subscribedMessages.count() != old_count)
                emit subscribedMessagesChanged(subscribedMessages());

            { // debug print
                const auto info = mavlink_get_message_info_by_id(id_num);
                const auto compName = appData->dialect()->componentName(type);
                if (info && compName) {
                    auto deb = qDebug().noquote() << compName << "in sys" << system.toString() << "comp" << component.toString();
                    if (result != MAV_RESULT_ACCEPTED) {
                        deb << "failed to setup";
                    } else if (interval == -1) {
                        deb << "unsubscribed from";
                    } else if (interval == 0) {
                        deb << "subscribed to";
                    } else {
                        deb << "limited rate to" << interval << "us for";
                    }
                    deb << info->name;
                }
            }
        }
    } else if (command == MAV_CMD_REQUEST_MESSAGE) {
        result = MAV_RESULT_DENIED; // assume parameters not accepted
        qint32 id_num = qRound(param1);
        int target = qRound(param7);
        if (id_num >= 0 && id_num <= 16777215 && target == 1) {
            // only allow requesting valid message id and for the client itself

            if (id_num == MAVLINK_MSG_ID_COMPONENT_INFORMATION_BASIC) {
                result = MAV_RESULT_ACCEPTED;
                sendMessage(appData->componentInformationBasic());
            }

            { // debug print
                const auto info = mavlink_get_message_info_by_id(id_num);
                const auto compName = appData->dialect()->componentName(type);
                if (info && compName) {
                    auto deb = qDebug().noquote()
                               << compName << "in sys" << system.toString() << "comp" << component.toString();
                    if (result != MAV_RESULT_ACCEPTED) {
                        deb << "requested unsupported message";
                    } else {
                        deb << "requested a message";
                    }
                    deb << info->name;
                }
            }
        }
    }

    mavlink_command_ack_t ack;
    ack.command = command;
    ack.result = result;
    ack.progress = 0;
    ack.result_param2 = 0;
    ack.target_system = message.senderSystem().value();
    ack.target_component = message.senderComponent().value();

    Message reply{Message::currentTime(), {}};
    mavlink_msg_command_ack_encode_chan(appData->localSystemId(),
                                        appData->localComponentId(),
                                        MAVLINK_COMM_0,
                                        &reply.m,
                                        &ack);
    sendMessage(reply);
}

void ClientNode::heartbeatTimerElapsed()
{
    if (_state != State::Unregistered) {
        _state = State::TimedOut;
        emit stateChanged(_state);
    }
}

const QMap<ComponentType, QSet<MessageId>> ClientNode::componentSubscriptions{
    {ComponentType(MARSH_TYPE_FLIGHT_MODEL),
     {
         MessageId(MAVLINK_MSG_ID_MANUAL_CONTROL),
     }},
    {ComponentType(MARSH_TYPE_INSTRUMENTS),
     {
         MessageId{MAVLINK_MSG_ID_MANUAL_CONTROL},
         MessageId(MAVLINK_MSG_ID_MANUAL_SETPOINT),
         MessageId(MAVLINK_MSG_ID_SIM_STATE),
         MessageId(MAVLINK_MSG_ID_ATTITUDE),
         MessageId(MAVLINK_MSG_ID_LOCAL_POSITION_NED),
         MessageId(MAVLINK_MSG_ID_HIGHRES_IMU),
     }},
    {ComponentType(MARSH_TYPE_VISUALISATION),
     {
         MessageId(MAVLINK_MSG_ID_SIM_STATE),
         MessageId(MAVLINK_MSG_ID_LOCAL_POSITION_NED),
         MessageId(MAVLINK_MSG_ID_ATTITUDE),
     }},
    {ComponentType(MARSH_TYPE_MOTION_PLATFORM),
     {
         MessageId(MAVLINK_MSG_ID_COMMAND_LONG),
         MessageId(MAVLINK_MSG_ID_COMMAND_INT),
         MessageId(MAVLINK_MSG_ID_SIM_STATE),
         MessageId(MAVLINK_MSG_ID_HIGHRES_IMU),
         MessageId(MAVLINK_MSG_ID_MOTION_CUE_EXTRA),
     }},
    {ComponentType(MARSH_TYPE_GSEAT),
     {
         MessageId(MAVLINK_MSG_ID_SIM_STATE),
         MessageId(MAVLINK_MSG_ID_HIGHRES_IMU),
         MessageId(MAVLINK_MSG_ID_MOTION_CUE_EXTRA),
     }},
};

QString ClientNode::Connection::toString()
{
    return QString("%1:%2").arg(address.toString()).arg(port);
}
