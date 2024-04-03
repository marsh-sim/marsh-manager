#include "parameterservice.h"
#include "applicationdata.h"
#include "router.h"
#include <cstring>

ParameterService::ParameterService(QObject *parent)
    : QObject{parent}
{}

void ParameterService::setAppData(ApplicationData *appData)
{
    this->appData = appData;

    // ask every new component for parameters
    connect(appData->router(), &Router::clientAdded, this, &ParameterService::clientAdded);
    // ask every component for parameters after log started
    connect(appData->logger(), &Logger::savingNowChanged, this, &ParameterService::savingChanged);
}

void ParameterService::requestAllParameters()
{
    const auto components = appData->router()->connectedComponents();
    for (auto it = components.cbegin(); it != components.cend(); it++) {
        mavlink_param_request_list_t request;
        request.target_component = it->value();
        request.target_system = appData->localSystemId();

        Message message{Message::currentTime(), {}};
        mavlink_msg_param_request_list_encode_chan(appData->localSystemId(),
                                                   appData->localComponentId(),
                                                   MAVLINK_COMM_0,
                                                   &message.m,
                                                   &request);

        appData->router()->sendMessage(message, *it, SystemId(appData->localSystemId()));
    }
}

void ParameterService::setParameter(QString id,
                                    QVariant value,
                                    ComponentId component,
                                    SystemId system)
{
    mavlink_param_set_t param_set;
    param_set.target_component = component.value();
    param_set.target_system = system.value();
    const auto buffer = id.toUtf8();
    std::memset(&param_set.param_id, 0, sizeof(param_set.param_id));
    std::memcpy(&param_set.param_id,
                buffer.data(),
                qMin(sizeof param_set.param_id, (unsigned long) buffer.size()));

    mavlink_param_union_t c_value;

    switch (value.typeId()) {
    case qMetaTypeId<quint8>():
        c_value.param_uint8 = value.toUInt();
        c_value.type = MAV_PARAM_TYPE_UINT8;
        break;
    case qMetaTypeId<qint8>():
        c_value.param_int8 = value.toInt();
        c_value.type = MAV_PARAM_TYPE_INT8;
        break;
    case qMetaTypeId<quint16>():
        c_value.param_uint16 = value.toUInt();
        c_value.type = MAV_PARAM_TYPE_UINT16;
        break;
    case qMetaTypeId<qint16>():
        c_value.param_int16 = value.toInt();
        c_value.type = MAV_PARAM_TYPE_INT16;
        break;
    case qMetaTypeId<quint32>():
        c_value.param_uint32 = value.toUInt();
        c_value.type = MAV_PARAM_TYPE_UINT32;
        break;
    case qMetaTypeId<qint32>():
        c_value.param_int32 = value.toInt();
        c_value.type = MAV_PARAM_TYPE_INT32;
        break;
    case qMetaTypeId<float>():
        c_value.param_float = value.toFloat();
        c_value.type = MAV_PARAM_TYPE_REAL32;
        break;
    default:
        qDebug() << "Unsupported QVariant for parameter value" << value;
        return;
    }

    param_set.param_value = c_value.param_float;
    param_set.param_type = c_value.type;

    Message message{Message::currentTime(), {}};
    mavlink_msg_param_set_encode_chan(appData->localSystemId(),
                                      appData->localComponentId(),
                                      MAVLINK_COMM_0,
                                      &message.m,
                                      &param_set);
    appData->router()->sendMessage(message, component, system);
}

void ParameterService::savingChanged(bool saving)
{
    if (saving) {
        requestAllParameters();
    }
}

void ParameterService::clientAdded(ClientNode *client)
{
    mavlink_param_request_list_t request;
    request.target_component = client->component.value();
    request.target_system = client->system.value();

    Message message{Message::currentTime(), {}};
    mavlink_msg_param_request_list_encode_chan(appData->localSystemId(),
                                               appData->localComponentId(),
                                               MAVLINK_COMM_0,
                                               &message.m,
                                               &request);

    appData->router()->sendMessage(message, client->component, client->system);
}
