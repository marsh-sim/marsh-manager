#include "message.h"

#include <QDateTime>

int64_t Message::currentTime()
{
    if (!Message::start_timestamp || !Message::running_timer) {
        start_timestamp = QDateTime::currentMSecsSinceEpoch() * 1000;
        running_timer = QElapsedTimer{};
        running_timer->start();
    }
    return *start_timestamp + running_timer->nsecsElapsed() / 1000;
}

std::optional<int64_t> Message::start_timestamp{};
std::optional<QElapsedTimer> Message::running_timer{};

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
