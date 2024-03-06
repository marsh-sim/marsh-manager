#include "clientnode.h"

ClientNode::ClientNode(QObject *parent, Connection connection)
    : QObject{parent}
    , _connection{connection}
{}

void ClientNode::receiveMessage(qint64 timestamp, mavlink_message_t message)
{
    last_published_message[message.msgid] = TimedMessage{timestamp, message};
}
