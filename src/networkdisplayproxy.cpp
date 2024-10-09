#include "networkdisplayproxy.h"
#include "applicationdata.h"
#include "networkdisplay.h"

NetworkDisplayProxy::NetworkDisplayProxy(QObject *parent)
    : QSortFilterProxyModel{parent}
{}

void NetworkDisplayProxy::setAppData(ApplicationData *appData)
{
    _networkDisplay = appData->networkDisplay();
    setSourceModel(_networkDisplay->model());
}

void NetworkDisplayProxy::hideCurrentlyTimedOut()
{
    if (!_networkDisplay)
        return;

    bool hiddenChanged = false;
    const auto &clientItems = _networkDisplay->clientItems;
    for (auto i = clientItems.cbegin(), end = clientItems.cend(); i != end; ++i) {
        if (i.key()->state() == ClientNode::State::TimedOut) {
            if (hideClient(i.key()))
                hiddenChanged = true;
        }
    }

    if (hiddenChanged)
        invalidateRowsFilter();
}

void NetworkDisplayProxy::showAllClients()
{
    if (hidden.empty())
        return;

    // do everything showClient() does
    for (auto i = hidden.cbegin(), end = hidden.cend(); i != end; ++i) {
        disconnect(*i, &ClientNode::stateChanged, this, &NetworkDisplayProxy::checkHiddenClients);
    }
    hidden.clear();
    emit hiddenClientCountChanged(0);
    invalidateRowsFilter();
}

int NetworkDisplayProxy::hiddenClientCount() const
{
    return hidden.count();
}

bool NetworkDisplayProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto rootIndex = _networkDisplay->model()->invisibleRootItem()->index();
    if (sourceParent != rootIndex)
        return true;

    QStandardItem *clientItem = _networkDisplay->model()->invisibleRootItem()->child(sourceRow);
    ClientNode *client = _networkDisplay->clientItems.key(clientItem, nullptr);
    Q_ASSERT_X(client,
               "NetworkDisplayProxy::filterAcceptsRow",
               "didn't find a matching client for the provided row and parent");

    return !hidden.contains(client);
}

void NetworkDisplayProxy::itemClicked(const QModelIndex &index)
{
    _networkDisplay->itemClicked(mapToSource(index));
}

void NetworkDisplayProxy::checkHiddenClients()
{
    bool hiddenChanged = false;

    // show all clients which are hidden now but not timed out
    for (auto i = hidden.cbegin(), end = hidden.cend(); i != end; ++i) {
        if ((*i)->state() != ClientNode::State::TimedOut) {
            if (showClient((*i)))
                hiddenChanged = true;
        }
    }

    if (hiddenChanged)
        invalidateRowsFilter();
}

bool NetworkDisplayProxy::hideClient(ClientNode *client)
{
    if (hidden.contains(client))
        return false;

    hidden.insert(client);
    connect(client, &ClientNode::stateChanged, this, &NetworkDisplayProxy::checkHiddenClients);

    emit hiddenClientCountChanged(hidden.count());
    return true;
}

bool NetworkDisplayProxy::showClient(ClientNode *client)
{
    if (!hidden.contains(client))
        return false;

    hidden.remove(client);
    disconnect(client, &ClientNode::stateChanged, this, &NetworkDisplayProxy::checkHiddenClients);

    emit hiddenClientCountChanged(hidden.count());
    return true;
}
