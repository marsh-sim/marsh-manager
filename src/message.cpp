#include "message.h"

#include <QDateTime>

int64_t Message::currentTime()
{
    if (!Message::startTimestamp || !Message::runningTimer) {
        startTimestamp = QDateTime::currentMSecsSinceEpoch() * 1000;
        runningTimer = QElapsedTimer{};
        runningTimer->start();
    }
    return *startTimestamp + runningTimer->nsecsElapsed() / 1000;
}

std::optional<int64_t> Message::startTimestamp{};
std::optional<QElapsedTimer> Message::runningTimer{};

int qHash(const SystemId &id)
{
    return qHash(id._value);
}

int qHash(const ComponentId &id)
{
    return qHash(id._value);
}

int qHash(const MessageId &id)
{
    return qHash(id._value);
}
