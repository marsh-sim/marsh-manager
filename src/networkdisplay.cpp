#include "networkdisplay.h"
#include <QApplication>
#include <QInputDialog>
#include <QMetaEnum>
#include <QRegularExpression>
#include <QTimer>
#include "applicationdata.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include <cmath>

NetworkDisplay::NetworkDisplay(QObject *parent)
    : QObject{parent}
    , startTimestamp{Message::currentTime()}
{
    _model = new QStandardItemModel(this);
    auto roleNames = _model->roleNames();
    roleNames[Qt::TextAlignmentRole] = "textAlign";
    roleNames[EditableRole] = "editable";
    _model->setItemRoleNames(roleNames);

    auto metaEnum = QMetaEnum::fromType<Column>();
    QStringList headerLabels{};
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        headerLabels.push_back(name(static_cast<Column>(i)));
    }
    _model->setHorizontalHeaderLabels(headerLabels);
}

void NetworkDisplay::setAppData(ApplicationData *appData)
{
    this->appData = appData;
    connect(appData->router(), &Router::clientAdded, this, &NetworkDisplay::addClient);
}

void NetworkDisplay::addClient(ClientNode *client)
{
    auto root = _model->invisibleRootItem();
    auto clientItem = new QStandardItem(
        QString("Client at %1").arg(client->connection().toString()));
    clientItem->setData(stateColor(client->state()), Qt::DecorationRole);

    auto updateItem = new QStandardItem(formatUpdateTime(Message::currentTime(), UpdateReason::Created));
    updateItem->setData(stateColor(client->state()), Qt::DecorationRole);
    updateItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);

    auto frequencyItem = new QStandardItem("");
    frequencyItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);

    auto dataItem = new QStandardItem(
        QString("System %1 %2").arg(client->system.toString(), name(client->component)));
    dataItem->setData(stateColor(client->state()), Qt::DecorationRole);

    clientItems[client] = clientItem;
    root->appendRow({clientItem, updateItem, frequencyItem, dataItem});

    auto stateItem = new QStandardItem(name(ClientRow::State));
    updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    updateItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    frequencyItem = new QStandardItem("");
    frequencyItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    dataItem = new QStandardItem(name(client->state()));
    dataItem->setData(stateColor(client->state()), Qt::DecorationRole);
    clientItem->appendRow({stateItem, updateItem, frequencyItem, dataItem});

    auto receivedItem = new QStandardItem(name(ClientRow::ReceivedMessages));
    updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    updateItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    frequencyItem = new QStandardItem("");
    frequencyItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    dataItem = new QStandardItem("");
    clientItem->appendRow({receivedItem, updateItem, frequencyItem, dataItem});

    auto sentItem = new QStandardItem(name(ClientRow::SentMessages));
    updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    updateItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    frequencyItem = new QStandardItem("");
    frequencyItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    dataItem = new QStandardItem("");
    clientItem->appendRow({sentItem, updateItem, frequencyItem, dataItem});

    auto paramItem = new QStandardItem(QString("No parameters"));
    paramItem->setData(stateColor(ClientNode::State::TimedOut), Qt::DecorationRole);
    updateItem = new QStandardItem("");
    updateItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    frequencyItem = new QStandardItem("");
    frequencyItem->setData(Qt::AlignRight, Qt::TextAlignmentRole);
    dataItem = new QStandardItem("");
    clientItem->appendRow({paramItem, updateItem, frequencyItem, dataItem});

    connect(client, &ClientNode::stateChanged, this, &NetworkDisplay::clientStateChanged);
    connect(client, &ClientNode::messageReceived, this, &NetworkDisplay::clientMessageReceived);
    connect(client, &ClientNode::messageSent, this, &NetworkDisplay::clientMessageSent);
}

void NetworkDisplay::itemClicked(const QModelIndex &index)
{
    int level = 0;
    auto parent = index.parent();
    while (parent.isValid()) {
        level++;
        parent = parent.parent();
    }

    // handle parameter value
    if (level == 2 && index.parent().row() == order(ClientRow::Parameters)
        && index.column() == order(Column::Data)) {
        const auto name = index.siblingAtColumn(order(Column::Name)).data(Qt::DisplayRole).toString();
        const auto valueText = index.data(Qt::DisplayRole).toString();
        const auto client = clientItems.key(_model->itemFromIndex(index.parent().parent()), nullptr);
        Q_ASSERT_X(client,
                   "NetworkDisplay::itemClicked",
                   "didn't find a matching client for the provided index");

        bool accepted;
        // TODO: Add dialog for integers
        double oldValue = valueText.toDouble();
        double newValue = QInputDialog::getDouble(qApp->activeWindow(),
                                                  QString("Edit %1").arg(name),
                                                  "New value:",
                                                  oldValue,
                                                  -2147483647, // default limit
                                                  2147483647,  // default limit
                                                  6,
                                                  &accepted);

        if (accepted) {
            appData->parameterService()->setParameter(name,
                                                      {static_cast<float>(newValue)},
                                                      client->component,
                                                      client->system);
        }
    }

    auto item = _model->itemFromIndex(index);
    auto deb = qDebug().nospace();
    deb << "Clicked item at L" << level << ", R" << index.row() << ", C" << index.column() << " { ";
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
        ->child(clientItem->row(), order(Column::Updated))
        ->setData(formatUpdateTime(Message::currentTime(),
                                   state == ClientNode::State::TimedOut ? UpdateReason::TimedOut : UpdateReason::StateChanged),
                  Qt::DisplayRole);

    const auto stateUpdated = clientItem->child(order(ClientRow::State), order(Column::Updated));
    stateUpdated->setData(formatUpdateTime(Message::currentTime()), Qt::DisplayRole);

    const auto stateData = clientItem->child(order(ClientRow::State), order(Column::Data));
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

    // update client interaction time only on receive for more intuitive overview
    if (direction == Direction::Received) {
        _model->invisibleRootItem()
            ->child(clientItem->row(), order(Column::Updated))
            ->setData(formatUpdateTime(message.timestamp,
                                       message.id() == MessageId(MAVLINK_MSG_ID_HEARTBEAT) ? UpdateReason::HeartbeatReceived : UpdateReason::Received),
                      Qt::DisplayRole);
        _model->invisibleRootItem()
            ->child(clientItem->row(), order(Column::Frequency))
            ->setData(client->receiveFrequency.formatFrequency(), Qt::DisplayRole);
    }

    const auto directionRow = direction == Direction::Received ? ClientRow::ReceivedMessages
                                                               : ClientRow::SentMessages;
    const auto directionFrequency = direction == Direction::Received ? client->receiveFrequency
                                                                     : client->sendFrequency;
    const auto directionHistory = direction == Direction::Received ? client->receivedMessages
                                                                   : client->sentMessages;

    // update time of any message in this direction
    clientItem->child(order(directionRow), order(Column::Updated))
        ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);
    clientItem->child(order(directionRow), order(Column::Frequency))
        ->setData(directionFrequency.formatFrequency(), Qt::DisplayRole);

    const auto directionItem = clientItem->child(order(directionRow));
    std::optional<int> messageRow;
    for (int row = 0; row < directionItem->rowCount(); ++row) {
        const auto item = directionItem->child(row, order(Column::Name));
        if (item->data(Qt::DisplayRole) == QString(info->name)) {
            messageRow = row;
        }
    }
    if (!messageRow) {
        // create cells for every column
        QList<QStandardItem *> created{};
        created.push_back(new QStandardItem(info->name));
        created.push_back(new QStandardItem(""));
        created.last()->setData(Qt::AlignRight, Qt::TextAlignmentRole);
        created.push_back(new QStandardItem(""));
        created.last()->setData(Qt::AlignRight, Qt::TextAlignmentRole);
        created.push_back(new QStandardItem(QString("id: ") + message.id().toString()));
        Q_ASSERT(created.size() == QMetaEnum::fromType<Column>().keyCount());

        // show the message source for sent messages
        if (direction == Direction::Sent) {
            auto dataItem = created.last();
            dataItem->setData(dataItem->data(Qt::DisplayRole).toString()
                                  + QString(", from system %1 %2")
                                        .arg(message.senderSystem().toString(),
                                             name(message.senderComponent())),
                              Qt::DisplayRole);
        }

        // insert in order of message id
        messageNameToId.insert(info->name, message.id());
        int insertRow = 0;
        for (int row = 0; row < directionItem->rowCount(); ++row) {
            const auto item = directionItem->child(row, order(Column::Name));
            if (message.id()
                < messageNameToId.value(item->data(Qt::DisplayRole).toString(), MessageId(0)))
                break; // shouldn't increase past this row
            insertRow++;
        }

        directionItem->insertRow(insertRow, created);
        messageRow = created[0]->row();
    }

    // update time for this message
    directionItem->child(*messageRow, order(Column::Updated))
        ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);
    directionItem->child(*messageRow, order(Column::Frequency))
        ->setData(directionHistory[message.id()].frequency.formatFrequency(), Qt::DisplayRole);

    auto messageItem = directionItem->child(*messageRow);
    if (!messageItem->hasChildren()) {
        // ensure there are rows for each field of the message in XML order
        for (int row = 0; row < info->num_fields; ++row) {
            QList<QStandardItem *> created{};
            const auto field = info->fields[row];
            created.push_back(new QStandardItem(field.name));
            created.push_back(new QStandardItem(formatUpdateTime(message.timestamp)));
            created.last()->setData(Qt::AlignRight, Qt::TextAlignmentRole);
            created.push_back(new QStandardItem(""));
            created.last()->setData(Qt::AlignRight, Qt::TextAlignmentRole);
            created.push_back(new QStandardItem(mavlinkData(field, message).toString()));
            Q_ASSERT(created.size() == QMetaEnum::fromType<Column>().keyCount());

            messageItem->appendRow(created);
        }
    }

    // update only changed values
    for (int row = 0; row < messageItem->rowCount(); ++row) {
        const auto field = info->fields[row];
        const auto data = formatFieldData(mavlinkData(field, message));
        if (data != messageItem->child(row, order(Column::Data))->data(Qt::DisplayRole)) {
            messageItem->child(row, order(Column::Data))->setData(data, Qt::DisplayRole);
            messageItem->child(row, order(Column::Updated))
                ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);
        }
    }

    if (direction == Direction::Received) {
        switch (message.m.msgid) {
        case MAVLINK_MSG_ID_PARAM_VALUE:
            handleParamValue(client, message);
            break;
        }
    }
}

void NetworkDisplay::handleParamValue(ClientNode *client, Message message)
{
    Q_ASSERT(message.id() == MessageId(MAVLINK_MSG_ID_PARAM_VALUE));
    const auto clientItem = clientItems[client];
    mavlink_param_value_t param_value;
    mavlink_msg_param_value_decode(&message.m, &param_value);

    // update parameter header
    const auto paramsItem = clientItem->child(order(ClientRow::Parameters));
    paramsItem->setData(name(ClientRow::Parameters), Qt::DisplayRole);
    paramsItem->setData(stateColor(ClientNode::State::Connected), Qt::DecorationRole);
    clientItem->child(order(ClientRow::Parameters), order(Column::Updated))
        ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);
    clientItem->child(order(ClientRow::Parameters), order(Column::Data))
        ->setData(QString("Count %1").arg(param_value.param_count), Qt::DisplayRole);

    // create empty rows up to index
    while (paramsItem->rowCount() <= param_value.param_index) {
        QList<QStandardItem *> created{};
        created.push_back(new QStandardItem(
            QString("(unknown parameter at index %1)").arg(paramsItem->rowCount())));
        created.push_back(new QStandardItem(""));
        created.last()->setData(Qt::AlignRight, Qt::TextAlignmentRole);
        created.push_back(new QStandardItem(""));
        created.last()->setData(Qt::AlignRight, Qt::TextAlignmentRole);
        created.push_back(new QStandardItem(""));
        Q_ASSERT(created.size() == QMetaEnum::fromType<Column>().keyCount());

        paramsItem->insertRow(paramsItem->rowCount(), created);
    }

    paramsItem->child(param_value.param_index, order(Column::Name))
        ->setData(QString(param_value.param_id), Qt::DisplayRole);
    paramsItem->child(param_value.param_index, order(Column::Updated))
        ->setData(formatUpdateTime(message.timestamp), Qt::DisplayRole);

    const auto dataItem = paramsItem->child(param_value.param_index, order(Column::Data));
    // FIXME: Store the initial QVariant instead of QString, format floating point numbers in QML delegate
    dataItem->setData(formatFieldData(paramData(param_value.param_value, param_value.param_type)),
                      Qt::DisplayRole);
    dataItem->setData(true, EditableRole);
}

QString NetworkDisplay::formatFieldData(QVariant data)
{
    // the approximate number of significant places is 7.2 for float and 16 for double
    // roughly estimated from mantissa width as log10(2^24) and log10(2^53)
    if (data.metaType() == QMetaType::fromType<float>()) {
        return QString("%1").arg(data.toFloat(), 0, 'f', 6);
    } else if (data.metaType() == QMetaType::fromType<double>()) {
        return QString("%1").arg(data.toDouble(), 0, 'f', 8);
    } else {
        return data.toString();
    }
}

QString NetworkDisplay::formatPascalCase(QString pascal)
{
    static const auto re = QRegularExpression("([a-z])([A-Z])");
    return pascal.left(1) + pascal.mid(1).replace(re, R"(\1 \2)").toLower();
}

QString NetworkDisplay::formatUpdateTime(qint64 timestamp, UpdateReason reason)
{
    double seconds = (timestamp - startTimestamp) / 1e6;
    const auto time = QString("%1:%2")
        .arg(static_cast<int>(std::floor(seconds / 60.0)), 2, 10, QChar('0'))
        .arg(QString("%1").arg(std::fmod(seconds, 60.0), 0, 'f', 3).rightJustified(6, '0'));

    switch (reason) {
    case UpdateReason::Created:
        return QString("Cr ") + time;
    case UpdateReason::Received:
        return QString("RX ") + time;
    case UpdateReason::TimedOut:
        return QString("TO ") + time;
    case UpdateReason::StateChanged:
        return QString("SC ") + time;
    case UpdateReason::HeartbeatReceived:
        return QString("HB ") + time;
    case UpdateReason::None:
    default:
        return time;
    }
}

int NetworkDisplay::order(Column value)
{
    return static_cast<int>(value);
}

int NetworkDisplay::order(ClientRow value)
{
    return static_cast<int>(value);
}

QString NetworkDisplay::name(Column value)
{
    auto metaEnum = QMetaEnum::fromType<Column>();
    return formatPascalCase(metaEnum.valueToKey(static_cast<int>(value)));
}

QString NetworkDisplay::name(ClientRow value)
{
    auto metaEnum = QMetaEnum::fromType<ClientRow>();
    return formatPascalCase(metaEnum.valueToKey(static_cast<int>(value)));
}

QString NetworkDisplay::name(ClientNode::State value)
{
    auto metaEnum = QMetaEnum::fromType<ClientNode::State>();
    QString name = formatPascalCase(metaEnum.valueToKey(static_cast<int>(value)));
    if (value == ClientNode::State::Unregistered) {
        // HACK: This way the hint will be consistent, name() for state is only used for the display role anyway
        name += QString(", please send HEARTBEAT message");
    }
    return name;
}

QString NetworkDisplay::name(ComponentId component)
{
    auto name = appData->dialect()->componentName(component);
    if (name)
        return *name;
    return QString("Component %1").arg(component.toString());
}

QVariant NetworkDisplay::stateColor(ClientNode::State state)
{
    // For now they need to work with both light and dark background
    switch (state) {
    case ClientNode::State::Connected:
        return {};
    case ClientNode::State::Shadowed:
        return QColor(63, 63, 191);
    case ClientNode::State::TimedOut:
        return QColor(127, 127, 127);
    case ClientNode::State::Unregistered:
        return QColor(171, 114, 0);
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

QVariant NetworkDisplay::paramData(const float param_value, const uint8_t param_type)
{
    mavlink_param_union_t value{param_value, param_type};

    switch (value.type) {
    case MAV_PARAM_TYPE_UINT8:
        return value.param_uint8;
    case MAV_PARAM_TYPE_INT8:
        return value.param_int8;
    case MAV_PARAM_TYPE_UINT16:
        return value.param_uint16;
    case MAV_PARAM_TYPE_INT16:
        return value.param_int16;
    case MAV_PARAM_TYPE_UINT32:
        return value.param_uint32;
    case MAV_PARAM_TYPE_INT32:
        return value.param_int32;
    case MAV_PARAM_TYPE_REAL32:
        return value.param_float;

    // these cases are larger than the value field in PARAM_VALUE message
    case MAV_PARAM_TYPE_UINT64:
    case MAV_PARAM_TYPE_INT64:
    case MAV_PARAM_TYPE_REAL64:
    default:
        return {};
    }
}
