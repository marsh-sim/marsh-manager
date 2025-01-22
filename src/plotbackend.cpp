#include "plotbackend.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include "networkdisplay.h"

PlotBackend::PlotBackend(QObject *parent)
    : QObject{parent}
{}

bool PlotBackend::addPath(QString path)
{
    if (_plottedPaths.contains(path))
        return false;

    _plottedPaths.insert(path);
    emit plottedPathsChanged(plottedPaths());
    return true;
}

bool PlotBackend::removePath(QString path)
{
    if (!_plottedPaths.contains(path))
        return false;

    _plottedPaths.remove(path);
    emit plottedPathsChanged(plottedPaths());
    return true;
}

void PlotBackend::addClient(ClientNode * const client)
{
    clients.insert(client->connection().toString(), client);
}

void PlotBackend::clientMessageReceived(Message message)
{
    // HACK: getting sender is not recommended, won't bee needed after rework to QAbstractModel
    const auto sender = qobject_cast<ClientNode *>(QObject::sender());
    Q_ASSERT_X(sender,
               "PlotBackend::clientMessageReceived",
               "this signal must be sent by a client");

    handleEvent(sender, Event::Received, message);
    if (message.id() == MessageId(MAVLINK_MSG_ID_PARAM_VALUE))
        handleEvent(sender, Event::Parameter, message);
}

void PlotBackend::clientMessageSent(Message message)
{
    // HACK: getting sender is not recommended, won't bee needed after rework to QAbstractModel
    const auto sender = qobject_cast<ClientNode *>(QObject::sender());
    Q_ASSERT_X(sender, "PlotBackend::clientMessageSent", "this signal must be sent by a client");
    handleEvent(sender, Event::Sent, message);
}

void PlotBackend::handleEvent(ClientNode * const client, Event event, Message message)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Event>();
    const auto eventPath = QString("%1/%2/")
                               .arg(client->connection().toString())
                               .arg(metaEnum.valueToKey(static_cast<int>(event)));
    if (event == Event::Parameter) {
        Q_ASSERT(message.id() == MessageId(MAVLINK_MSG_ID_PARAM_VALUE));
        mavlink_param_value_t param_value;
        mavlink_msg_param_value_decode(&message.m, &param_value);

        const auto fullPath = eventPath + NetworkDisplay::paramName(param_value.param_id);
        const auto data = NetworkDisplay::paramData(param_value.param_value, param_value.param_type);
        handleData(fullPath, message.timestamp, data);
    } else {
        const auto info = mavlink_get_message_info(&message.m);
        for (int fieldIndex = 0; fieldIndex < info->num_fields; ++fieldIndex) {
            const auto field = info->fields[fieldIndex];

            const auto fullPath = eventPath + QString(field.name);
            const auto data = NetworkDisplay::mavlinkData(field, message);
            handleData(fullPath, message.timestamp, data);
        }
    }
}

void PlotBackend::handleData(QString path, int64_t timestamp, QVariant data)
{
    if (!_plottedPaths.contains(path))
        return;

    // TODO: Manage the lineseries
}
