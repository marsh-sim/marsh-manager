#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QObject>
#include <QStandardItemModel>
#include "clientnode.h"

/// Translates application types into standard Qt model class
class NetworkDisplay : public QObject
{
    Q_OBJECT
public:
    /// Columns defined for each item
    enum class Column : int {
        Name,
        Updated,
        Data,
    };
    Q_ENUM(Column)

    /// Rows present on each client
    enum class ClientRow : int {
        State,
        ReceivedMessages,
    };
    Q_ENUM(ClientRow)

    explicit NetworkDisplay(QObject *parent = nullptr);

    Q_PROPERTY(QStandardItemModel *model READ model CONSTANT)

    QStandardItemModel *model() const { return _model; }

    void addClient(ClientNode *client);
signals:

public slots:
    void itemClicked(const QModelIndex &index);

private slots:
    void clientStateChanged(ClientNode::State state);
    void clientMessageReceived(Message message);

private:
    int index(Column column);
    int index(ClientRow row);
    QString name(Column column);
    QString name(ClientRow row);
    QString name(ClientNode::State state);

    QString formatUpdateTime(qint64 timestamp);
    QVariant stateColor(ClientNode::State state);
    QVariant mavlinkData(const mavlink_field_info_t &field, const Message &message);

    QStandardItemModel *_model;
    const qint64 startTimestamp;

    QMap<ClientNode *, QStandardItem *> clientItems;
};

#endif // NETWORKMODEL_H
