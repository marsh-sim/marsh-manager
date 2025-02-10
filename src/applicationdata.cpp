#include "applicationdata.h"

#include <QFile>
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect

ApplicationData::ApplicationData(QObject *parent)
    : QObject{parent}
{
    _router = new Router(this);
    _networkDisplay = new NetworkDisplay(this);
    _displayModel = new NetworkDisplayProxy(this);
    _logger = new Logger(this);
    _heartbeatService = new HeartbeatService(this);
    _parameterService = new ParameterService(this);
    _dialect = new DialectInfo(this);

    // pass this reference again when all children are constructed
    _router->setAppData(this);
    _networkDisplay->setAppData(this);
    _displayModel->setAppData(this);
    _logger->setAppData(this);
    _heartbeatService->setAppData(this);
    _parameterService->setAppData(this);
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

Message ApplicationData::componentInformationBasic() const
{
    mavlink_component_information_basic_t info;
    info.time_boot_ms = Message::timeBootMs();
    info.capabilities = MAV_PROTOCOL_CAPABILITY_COMMAND_INT | MAV_PROTOCOL_CAPABILITY_MAVLINK2;
    info.time_manufacture_s = kGitEpoch;
    std::memset(info.vendor_name, 0, sizeof(info.vendor_name));
    std::memset(info.model_name, 0, sizeof(info.model_name));
    std::memset(info.software_version, 0, sizeof(info.software_version));
    std::strncpy(info.software_version,
                 QCoreApplication::applicationVersion().toUtf8(),
                 sizeof(info.software_version));
    std::memset(info.hardware_version, 0, sizeof(info.hardware_version));
    std::memset(info.serial_number, 0, sizeof(info.serial_number));

    Message message{Message::currentTime(), {}};
    mavlink_msg_component_information_basic_encode_chan(localSystemId(),
                                                        localComponentId(),
                                                        MAVLINK_COMM_0,
                                                        &message.m,
                                                        &info);
    return message;
}
