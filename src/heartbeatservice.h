#ifndef HEARTBEATSERVICE_H
#define HEARTBEATSERVICE_H

#include <QObject>
#include <QTimer>

class ApplicationData;
class Router;

/// Implements the Heartbeat/Connection Protocol for the Manager application
/// See documentation at https://mavlink.io/en/services/heartbeat.html
class HeartbeatService : public QObject
{
    Q_OBJECT
public:
    explicit HeartbeatService(QObject *parent = nullptr);
    void setAppData(ApplicationData *appData);

signals:

private slots:
    void sendTimerElapsed();

private:
    ApplicationData *appData;

    QTimer *sendTimer;
};

#endif // HEARTBEATSERVICE_H
