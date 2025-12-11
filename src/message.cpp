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

int32_t Message::timeBootMs()
{
    if (!Message::startTimestamp || !Message::runningTimer) {
        startTimestamp = QDateTime::currentMSecsSinceEpoch() * 1000;
        runningTimer = QElapsedTimer{};
        runningTimer->start();
    }
    return runningTimer->elapsed();
}

std::optional<int64_t> Message::startTimestamp{};
std::optional<QElapsedTimer> Message::runningTimer{};

const SystemId SystemId::Broadcast{0};

int qHash(const SystemId &id)
{
    return qHash(id._value);
}

const ComponentId ComponentId::Broadcast{0};

int qHash(const ComponentId &id)
{
    return qHash(id._value);
}

const ComponentType ComponentType::Broadcast{0};

int qHash(const ComponentType &type)
{
    return qHash(type._value);
}

int qHash(const MessageId &id)
{
    return qHash(id._value);
}
