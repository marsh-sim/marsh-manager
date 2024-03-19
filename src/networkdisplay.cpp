#include "networkdisplay.h"
#include <QMetaEnum>
#include <QTimer>
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include <cmath>

NetworkDisplay::NetworkDisplay(QObject *parent)
    : QObject{parent}
    , startTimestamp{Message::currentTime()}
{
    _model = new QStandardItemModel(this);

    auto metaEnum = QMetaEnum::fromType<Column>();
    QStringList headerLabels{};
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        headerLabels.push_back(name(static_cast<Column>(i)));
    }
    _model->setHorizontalHeaderLabels(headerLabels);
}

void NetworkDisplay::addClient(ClientNode *client)
{
    auto root = _model->invisibleRootItem();
    auto clientItem = new QStandardItem(
        QString("Client at %1").arg(client->connection().toString()));
    clientItem->setData(stateColor(client->state()), Qt::DecorationRole);

    auto updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));

    auto dataItem = new QStandardItem(
        QString("System %1 component %2")
            .arg(client->system.toString(), client->component.toString()));

    root->appendRow({clientItem, updateItem, dataItem});
    clientItems[client] = clientItem;

    auto stateItem = new QStandardItem(name(ClientRow::State));
    updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    dataItem = new QStandardItem(name(client->state()));
    dataItem->setData(stateColor(client->state()), Qt::DecorationRole);
    clientItem->appendRow({stateItem, updateItem, dataItem});

    auto receivedItem = new QStandardItem(name(ClientRow::ReceivedMessages));
    updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    dataItem = new QStandardItem("");
    clientItem->appendRow({receivedItem, updateItem, dataItem});

    auto sentItem = new QStandardItem(name(ClientRow::SentMessages));
    updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    dataItem = new QStandardItem("");
    clientItem->appendRow({sentItem, updateItem, dataItem});

    connect(client, &ClientNode::stateChanged, this, &NetworkDisplay::clientStateChanged);
    connect(client, &ClientNode::messageReceived, this, &NetworkDisplay::clientMessageReceived);
    connect(client, &ClientNode::messageSent, this, &NetworkDisplay::clientMessageSent);
}

void NetworkDisplay::itemClicked(const QModelIndex &index)
{
    auto item = _model->itemFromIndex(index);
    auto deb = qDebug().nospace();
    deb << "Clicked item at " << index.row() << ", " << index.column() << " { ";
    for (const auto [role, name] : _model->roleNames().asKeyValueRange()) {
        deb << name << ": " << item->data(role) << ", ";
    }
    deb << "}";
}

void NetworkDisplay::clientStateChanged(ClientNode::State state)
{
    // HACK: getting sender is not recommended, won't bee needed after rework to QAbstractModel
    auto sender = qobject_cast<ClientNode *>(QObject::sender());
    Q_ASSERT_X(sender, "NetworkDisplay::clientStateChanged", "this signal must be sent by a client");

    const auto clientItem = clientItems[sender];
    // update color of whole client row
    for (int col = 0; col < _model->invisibleRootItem()->columnCount(); ++col) {
        _model->invisibleRootItem()
            ->child(clientItem->row(), col)
            ->setData(stateColor(state), Qt::DecorationRole);
    }

    // update client interaction time
    _model->invisibleRootItem()
        ->child(clientItem->row(), index(Column::Updated))
        ->setData(formatUpdateTime(Message::currentTime()), Qt::DisplayRole);

    const auto stateUpdated = clientItem->child(index(ClientRow::State), index(Column::Updated));
    stateUpdated->setData(formatUpdateTime(Message::currentTime()), Qt::DisplayRole);

    const auto stateData = clientItem->child(index(ClientRow::State), index(Column::Data));
    stateData->setData(name(state), Qt::DisplayRole);
    stateData->setData(stateColor(state), Qt::DecorationRole);
}

void NetworkDisplay::clientMessageReceived(Message message)
{
    // HACK: getting sender is not recommended, won't bee needed after rework to QAbstractModel
    const auto sender = qobject_cast<ClientNode *>(QObject::sender());
    Q_ASSERT_X(sender,
               "NetworkDisplay::clientMessageReceived",
               "this signal must be sent by a client");
    handleClientMessage(sender, message, Direction::Received);
}

void NetworkDisplay::clientMessageSent(Message message)
{
    // HACK: getting sender is not recommended, won't bee needed after rework to QAbstractModel
    const auto sender = qobject_cast<ClientNode *>(QObject::sender());
    Q_ASSERT_X(sender, "NetworkDisplay::clientMessageSent", "this signal must be sent by a client");
    handleClientMessage(sender, message, Direction::Sent);
}

void NetworkDisplay::handleClientMessage(ClientNode *client, Message message, Direction direction)
{
    const auto clientItem = clientItems[client];
    const auto info = mavlink_get_message_info(&message.m);

    // update client interaction time
    _model->invisibleRootItem()
        ->child(clientItem->row(), index(Column::Updated))
        ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);
    // update any received message time
    clientItem->child(index(ClientRow::ReceivedMessages), index(Column::Updated))
        ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);

    const auto directionRow = direction == Direction::Received ? ClientRow::ReceivedMessages
                                                               : ClientRow::SentMessages;
    const auto directionItem = clientItem->child(index(directionRow));
    std::optional<int> messageRow;
    for (int row = 0; row < directionItem->rowCount(); ++row) {
        const auto item = directionItem->child(row, index(Column::Name));
        if (item->data(Qt::DisplayRole) == QString(info->name)) {
            messageRow = row;
        }
    }
    if (!messageRow) {
        // create cells for every column
        QList<QStandardItem *> created{};
        created.push_back(new QStandardItem(info->name));
        created.push_back(new QStandardItem(""));
        created.push_back(new QStandardItem(QString("id: ") + message.id().toString()));

        // show the message source for sent messages
        if (direction == Direction::Sent) {
            auto dataItem = created.last();
            dataItem->setData(dataItem->data(Qt::DisplayRole).toString()
                                  + QString(", from system %1 component %2")
                                        .arg(message.senderSystem().toString())
                                        .arg(message.senderComponent().toString()),
                              Qt::DisplayRole);
        }

        // insert in order of message id
        messageNameToId.insert(info->name, message.id());
        int insertRow = 0;
        for (int row = 0; row < directionItem->rowCount(); ++row) {
            const auto item = directionItem->child(row, index(Column::Name));
            if (message.id()
                < messageNameToId.value(item->data(Qt::DisplayRole).toString(), MessageId(0)))
                break; // shouldn't increase past this row
            insertRow++;
        }

        directionItem->insertRow(insertRow, created);
        messageRow = created[0]->row();
    }

    // update time for this message
    directionItem->child(*messageRow, index(Column::Updated))
        ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);

    auto messageItem = directionItem->child(*messageRow);
    if (!messageItem->hasChildren()) {
        // ensure there are rows for each field of the message in XML order
        for (int row = 0; row < info->num_fields; ++row) {
            QList<QStandardItem *> created{};
            const auto field = info->fields[row];
            created.push_back(new QStandardItem(field.name));
            created.push_back(new QStandardItem(formatUpdateTime(message.timestamp)));
            created.push_back(new QStandardItem(mavlinkData(field, message).toString()));

            messageItem->appendRow(created);
        }
    }

    // update only changed values
    for (int row = 0; row < messageItem->rowCount(); ++row) {
        const auto field = info->fields[row];
        auto data = mavlinkData(field, message).toString();
        if (data != messageItem->child(row, index(Column::Data))->data(Qt::DisplayRole)) {
            messageItem->child(row, index(Column::Data))->setData(data, Qt::DisplayRole);
            messageItem->child(row, index(Column::Updated))
                ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);
        }
    }
}

QString NetworkDisplay::formatUpdateTime(qint64 timestamp)
{
    double seconds = (timestamp - startTimestamp) / 1e6;
    return QString("%1:%2")
        .arg(static_cast<int>(std::floor(seconds / 60.0)), 2, 10, QChar('0'))
        .arg(QString("%1").arg(std::fmod(seconds, 60.0), 0, 'f', 3).rightJustified(6, '0'));
}

int NetworkDisplay::index(Column value)
{
    return static_cast<int>(value);
}

int NetworkDisplay::index(ClientRow value)
{
    return static_cast<int>(value);
}

QString NetworkDisplay::name(Column value)
{
    auto metaEnum = QMetaEnum::fromType<Column>();
    return QString(metaEnum.valueToKey(static_cast<int>(value)));
}

QString NetworkDisplay::name(ClientRow value)
{
    auto metaEnum = QMetaEnum::fromType<ClientRow>();
    return QString(metaEnum.valueToKey(static_cast<int>(value)));
}

QString NetworkDisplay::name(ClientNode::State value)
{
    auto metaEnum = QMetaEnum::fromType<ClientNode::State>();
    return QString(metaEnum.valueToKey(static_cast<int>(value)));
}

QVariant NetworkDisplay::stateColor(ClientNode::State state)
{
    switch (state) {
    case ClientNode::State::Connected:
        return {};
    case ClientNode::State::Shadowed:
        return QColor(63, 63, 191);
    case ClientNode::State::TimedOut:
        return QColor(127, 127, 127);
    }
    qWarning() << "NetworkDisplay::stateColor didn't handle state" << name(state);
    return {};
}

QVariant NetworkDisplay::mavlinkData(const mavlink_field_info_t &field, const Message &message)
{
    switch (field.type) {
    case MAVLINK_TYPE_CHAR:
        return _MAV_RETURN_char(&message.m, field.wire_offset);
    case MAVLINK_TYPE_UINT8_T:
        return _MAV_RETURN_uint8_t(&message.m, field.wire_offset);
    case MAVLINK_TYPE_INT8_T:
        return _MAV_RETURN_int8_t(&message.m, field.wire_offset);
    case MAVLINK_TYPE_UINT16_T:
        return _MAV_RETURN_uint16_t(&message.m, field.wire_offset);
    case MAVLINK_TYPE_INT16_T:
        return _MAV_RETURN_int16_t(&message.m, field.wire_offset);
    case MAVLINK_TYPE_UINT32_T:
        return _MAV_RETURN_uint32_t(&message.m, field.wire_offset);
    case MAVLINK_TYPE_INT32_T:
        return _MAV_RETURN_int32_t(&message.m, field.wire_offset);
    case MAVLINK_TYPE_UINT64_T:
        return static_cast<quint64>(_MAV_RETURN_uint64_t(&message.m, field.wire_offset));
    case MAVLINK_TYPE_INT64_T:
        return static_cast<qint64>(_MAV_RETURN_int64_t(&message.m, field.wire_offset));
    case MAVLINK_TYPE_FLOAT:
        return _MAV_RETURN_float(&message.m, field.wire_offset);
    case MAVLINK_TYPE_DOUBLE:
        return _MAV_RETURN_double(&message.m, field.wire_offset);
    }
    qWarning() << "NetworkDisplay::mavlinkData didn't handle MAVLINK_TYPE"
               << static_cast<int>(field.type);
    return {};
}
