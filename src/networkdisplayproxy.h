#ifndef NETWORKDISPLAYPROXY_H
#define NETWORKDISPLAYPROXY_H

#include <QObject>
#include <QSortFilterProxyModel>
#include "clientnode.h"

class ApplicationData;
class NetworkDisplay;

class NetworkDisplayProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit NetworkDisplayProxy(QObject *parent = nullptr);
    void setAppData(ApplicationData *appData);

    Q_PROPERTY(int hiddenClientCount READ hiddenClientCount NOTIFY hiddenClientCountChanged FINAL)

    Q_INVOKABLE void hideCurrentlyTimedOut();
    Q_INVOKABLE void showAllClients();

    int hiddenClientCount() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

public slots:
    void itemClicked(const QModelIndex &index);

private slots:
    /// Show clients that come back from being timed out
    void checkHiddenClients();

signals:
    void hiddenClientCountChanged(int count);

private:
    /// Returns whether any change was made
    bool hideClient(ClientNode *client);
    /// Returns whether any change was made
    bool showClient(ClientNode *client);

    NetworkDisplay *_networkDisplay;

    /// Do not modify directly, call hideClient or showClient
    QSet<ClientNode *> hidden;
};

#endif // NETWORKDISPLAYPROXY_H
