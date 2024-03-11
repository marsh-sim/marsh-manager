#include "networkdisplay.h"
#include <QMetaEnum>
#include <QTimer>
#include <cmath>

NetworkDisplay::NetworkDisplay(QObject *parent)
    : QObject{parent}
    , startTimestamp{Message::currentTime()}
{
    _model = new QStandardItemModel(this);
    _model->setHorizontalHeaderLabels({"Name", "Updated", "Data"});
}

void NetworkDisplay::addClient(ClientNode *client)
{
    auto root = _model->invisibleRootItem();
    auto clientItem = new QStandardItem(
        QString("Client at %1").arg(client->connection().toString()));
    auto updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    auto dataItem = new QStandardItem(
        QString("System %1 Component %2")
            .arg(client->system.toString(), client->component.toString()));
    root->appendRow({clientItem, updateItem, dataItem});
    clientItems[client] = clientItem;

    auto stateItem = new QStandardItem(QString("state"));
    updateItem = new QStandardItem(formatUpdateTime(Message::currentTime()));
    dataItem = new QStandardItem(stateName(client->state()));
    clientItem->appendRow({stateItem, updateItem, dataItem});

    connect(client, &ClientNode::stateChanged, this, &NetworkDisplay::clientStateChanged);
    connect(client, &ClientNode::messageReceived, this, &NetworkDisplay::clientMessageReceived);
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
    Q_ASSERT(clientItem->child(0, 0)->data(Qt::DisplayRole) == QVariant(QString("state")));

    const auto stateUpdated = clientItem->child(0, 1);
    stateUpdated->setData(formatUpdateTime(Message::currentTime()), Qt::DisplayRole);

    const auto stateData = clientItem->child(0, 2);
    stateData->setData(stateName(state), Qt::DisplayRole);
}

void NetworkDisplay::clientMessageReceived(Message message)
{
    // TODO: implement
}

QString NetworkDisplay::formatUpdateTime(qint64 timestamp)
{
    double seconds = (timestamp - startTimestamp) / 1e6;
    return QString("%1:%2")
        .arg(static_cast<int>(std::floor(seconds / 60.0)), 2, 10, QChar('0'))
        .arg(QString("%1").arg(std::fmod(seconds, 60.0), 0, 'f', 3).rightJustified(6, '0'));
}

QString NetworkDisplay::stateName(ClientNode::State state)
{
    auto metaEnum = QMetaEnum::fromType<ClientNode::State>();
    return QString(metaEnum.valueToKey(static_cast<int>(state)));
}
