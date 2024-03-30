#ifndef PARAMETERSERVICE_H
#define PARAMETERSERVICE_H

#include <QObject>

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
signals:

private slots:
    void savingChanged(bool saving);
    void clientAdded(ClientNode *client);

private:
    ApplicationData *appData;
};

#endif // PARAMETERSERVICE_H
