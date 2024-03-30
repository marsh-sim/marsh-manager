#include "heartbeatservice.h"
#include "applicationdata.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect
#include "router.h"

HeartbeatService::HeartbeatService(QObject *parent)
    : QObject{parent}
{
    sendTimer = new QTimer(this);
    sendTimer->setSingleShot(false);
    // send at 1Hz
    sendTimer->setInterval(1000);
    sendTimer->start();
    connect(sendTimer, &QTimer::timeout, this, &HeartbeatService::sendTimerElapsed);
}

void HeartbeatService::setAppData(ApplicationData *appData)
{
    this->appData = appData;
}

void HeartbeatService::sendTimerElapsed()
{
    mavlink_heartbeat_t heartbeat;
    heartbeat.type = MAV_TYPE_GCS;
    heartbeat.autopilot = MAV_AUTOPILOT_INVALID;
    heartbeat.base_mode = 0; // none of the flags applicable
    // TODO: Consider sending information about active logging either in custom_mode or system_status
    heartbeat.custom_mode = 0; // not used so far
    heartbeat.system_status = MAV_STATE_ACTIVE;

    Message message{Message::currentTime(), {}};
    mavlink_msg_heartbeat_encode_chan(appData->localSystemId(),
                                      appData->localComponentId(),
                                      MAVLINK_COMM_0,
                                      &message.m,
                                      &heartbeat);

    appData->router()->sendMessage(message);
}
