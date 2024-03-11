#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QObject>
#include <QStandardItemModel>
#include "clientnode.h"
#include <optional>

class NetworkDisplay : public QObject
{
    Q_OBJECT
public:
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
    QString formatUpdateTime(qint64 timestamp);
    QString stateName(ClientNode::State state);
    QVariant stateColor(ClientNode::State state);

    QStandardItemModel *_model;
    const qint64 startTimestamp;

    QMap<ClientNode *, QStandardItem *> clientItems;
};

#endif // NETWORKMODEL_H
