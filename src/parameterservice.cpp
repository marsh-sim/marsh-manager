#include "parameterservice.h"
#include "applicationdata.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include "router.h"

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
