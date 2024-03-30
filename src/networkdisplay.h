#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QObject>
#include <QStandardItemModel>
#include "clientnode.h"

class ApplicationData;

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
        SentMessages,
    };
    Q_ENUM(ClientRow)

    explicit NetworkDisplay(QObject *parent = nullptr);
    void setAppData(ApplicationData *appData);

    Q_PROPERTY(QStandardItemModel *model READ model CONSTANT)

    Q_INVOKABLE QString formatUpdateTime(qint64 timestamp);

    QStandardItemModel *model() const { return _model; }

signals:

public slots:
    void itemClicked(const QModelIndex &index);

private slots:
    void addClient(ClientNode *client);
    void clientStateChanged(ClientNode::State state);
    void clientMessageReceived(Message message);
    void clientMessageSent(Message message);

private:
    enum class Direction {
        Received,
        Sent,
    };

    void handleClientMessage(ClientNode *client, Message message, Direction direction);
    QString formatFieldData(QVariant data);
    QString formatPascalCase(QString pascal);

    int index(Column column);
    int index(ClientRow row);
    QString name(Column column);
    QString name(ClientRow row);
    QString name(ClientNode::State state);
    QString name(ComponentId component);

    QVariant stateColor(ClientNode::State state);
    QVariant mavlinkData(const mavlink_field_info_t &field, const Message &message);

    ApplicationData *appData;
    QStandardItemModel *_model;
    const qint64 startTimestamp;

    QMap<ClientNode *, QStandardItem *> clientItems;
    QMap<QString, MessageId> messageNameToId;
};

#endif // NETWORKMODEL_H
