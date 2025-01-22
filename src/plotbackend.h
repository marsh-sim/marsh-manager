#ifndef PLOTBACKEND_H
#define PLOTBACKEND_H

#include <QObject>
#include <QValueAxis>
#include "clientnode.h"

class PlotBackend : public QObject
{
    Q_OBJECT
public:
    explicit PlotBackend(QObject *parent = nullptr);

    /// Kind of data received
    enum class Event {
        Received,
        Sent,
        Parameter,
    };
    Q_ENUM(Event)

    Q_PROPERTY(QStringList plottedPaths READ plottedPaths NOTIFY plottedPathsChanged FINAL)
    Q_PROPERTY(QValueAxis* timeAxis READ timeAxis CONSTANT)

    QStringList plottedPaths() const { return _plottedPaths.values(); }
    QValueAxis* timeAxis() const { return _timeAxis; };

    Q_INVOKABLE bool addPath(QString path);
    Q_INVOKABLE bool removePath(QString path);

signals:
    void plottedPathsChanged(QStringList paths);

private slots:
    void addClient(ClientNode *const client);
    void clientMessageReceived(Message message);
    void clientMessageSent(Message message);

private:
    void handleEvent(ClientNode *const client, Event event, Message message);
    void handleData(QString path, int64_t timestamp, QVariant data);

    QSet<QString> _plottedPaths;
    QValueAxis* _timeAxis;

    QMap<QString, ClientNode*> clients;
};

#endif // PLOTBACKEND_H
