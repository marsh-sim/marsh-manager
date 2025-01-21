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
        Frequency,
        Data,
        Plotting,
    };
    Q_ENUM(Column)

    /// Rows present on each client
    enum class ClientRow : int {
        State,
        ReceivedMessages,
        SentMessages,
        Parameters,
        SubscribedMessages,
    };
    Q_ENUM(ClientRow)

    enum class UpdateReason {
        None,
        Created,
        Received,
        TimedOut,
        StateChanged,
        HeartbeatReceived,
    };
    Q_ENUM(UpdateReason)

    friend class NetworkDisplayProxy;

    explicit NetworkDisplay(QObject *parent = nullptr);
    void setAppData(ApplicationData *appData);

    Q_PROPERTY(QStandardItemModel *model READ model CONSTANT)

    Q_INVOKABLE QString formatUpdateTime(qint64 timestamp, UpdateReason reason=UpdateReason::None);

    QStandardItemModel *model() const { return _model; }

signals:

public slots:
    void itemClicked(const QModelIndex &index);

private slots:
    void addClient(ClientNode *const client);
    void clientStateChanged(ClientNode::State state);
    void clientMessageReceived(Message message);
    void clientMessageSent(Message message);
    void clientSubscriptionsChanged(QSet<MessageId> subs);

private:
    enum class Direction {
        Received,
        Sent,
    };

    enum CustomRoles : int {
        EditableRole = Qt::UserRole,
        PlottingRole,
    };

    void handleClientMessage(ClientNode *const client, Message message, Direction direction);
    void handleParamValue(ClientNode *const client, Message message);
    void updateSubscribed(ClientNode *const client);
    QString formatFieldData(QVariant data);
    QString formatPascalCase(QString pascal);

    int order(Column column);
    int order(ClientRow row);
    QString name(Column column);
    QString name(ClientRow row);
    QString name(ClientNode::State state);
    QString name(ComponentId component);

    QVariant stateColor(ClientNode::State state);
    QVariant mavlinkData(const mavlink_field_info_t &field, const Message &message);
    QVariant paramData(const float param_value, const uint8_t param_type);

    ApplicationData *appData;
    QStandardItemModel *_model;
    const qint64 startTimestamp;

    QMap<ClientNode *, QStandardItem *> clientItems;
    QMap<QString, MessageId> messageNameToId;
};

#endif // NETWORKMODEL_H
