#include "applicationdata.h"

#include <QFile>
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect

ApplicationData::ApplicationData(QObject *parent)
    : QObject{parent}
{
    _router = new Router(this);
    _networkDisplay = new NetworkDisplay(this);
    _logger = new Logger(this);
    _heartbeatService = new HeartbeatService(this);

    // pass this reference again when all children are constructed
    _router->setAppData(this);
    _logger->setAppData(this);
    _heartbeatService->setAppData(this);
}

quint8 ApplicationData::localSystemId() const
{
    return 1;
}
quint8 ApplicationData::localComponentId() const
{
    return MARSH_COMP_ID_MANAGER;
}

QString ApplicationData::licenseText()
{
    QString filename(":/LICENSE.txt");

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open license file";
        return "";
    }

    return file.readAll();
}
