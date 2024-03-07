#ifndef MESSAGE_H
#define MESSAGE_H

#include <QElapsedTimer>
#include "mavlink/mavlink_types.h"
#include <optional>

class SystemId
{
public:
    explicit SystemId(uint8_t value)
        : _value(value)
    {}

    bool operator==(const SystemId &other) const { return _value == other._value; }
    bool operator<(const SystemId &other) const { return _value < other._value; }
    friend int qHash(const SystemId &id);

private:
    uint8_t _value;
};
Q_DECLARE_TYPEINFO(SystemId, Q_PRIMITIVE_TYPE);

class ComponentId
{
public:
    explicit ComponentId(uint8_t value)
        : _value(value)
    {}

    bool operator==(const ComponentId &other) const { return _value == other._value; }
    bool operator<(const ComponentId &other) const { return _value < other._value; }
    friend int qHash(const ComponentId &id);

private:
    uint8_t _value;
};
Q_DECLARE_TYPEINFO(ComponentId, Q_PRIMITIVE_TYPE);

class MessageId
{
public:
    explicit MessageId(uint32_t value)
        : _value(value)
    {}

    bool operator==(const MessageId &other) const { return _value == other._value; }
    bool operator<(const MessageId &other) const { return _value < other._value; }
    friend int qHash(const MessageId &id);

private:
    uint32_t _value;
};
Q_DECLARE_TYPEINFO(MessageId, Q_PRIMITIVE_TYPE);

/// Convenience wrapper around MAVLink message with timestamp.
/// Provides strong typing for some properties
struct Message
{
    /// Microseconds since UNIX epoch.
    /// Time of message creation (time received in the manager)
    int64_t timestamp;
    /// Contained MAVLink as defined by the library
    mavlink_message_t m;

    MessageId id() const { return MessageId(m.msgid); }
    SystemId senderSystem() const { return SystemId(m.sysid); }
    ComponentId senderComponent() const { return ComponentId(m.compid); }

    /// Global timer for timestamps in microseconds since epoch
    static int64_t currentTime();

private:
    /// Singleton offset since epoch
    static std::optional<int64_t> start_timestamp;
    /// Singleton timer to get better accuracy than system clock
    static std::optional<QElapsedTimer> running_timer;
};

#endif // MESSAGE_H
