#ifndef PARAMETERSERVICE_H
#define PARAMETERSERVICE_H

#include <QObject>
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include "message.h"

class ApplicationData;
class Router;
class ClientNode;

/// Implements the Parameter Protocol for the Manager application
/// See documentation at https://mavlink.io/en/services/parameter.html
class ParameterService : public QObject
{
    Q_OBJECT
public:
    explicit ParameterService(QObject *parent = nullptr);
    void setAppData(ApplicationData *appData);

    void requestAllParameters();
    void setParameter(QString id,
                      QVariant value,
                      ComponentId component = ComponentId::Broadcast,
                      SystemId system = SystemId::Broadcast);
signals:

private slots:
    void savingChanged(bool saving);
    void clientAdded(ClientNode *client);

private:
    ApplicationData *appData;
};

#endif // PARAMETERSERVICE_H
