#pragma once

namespace swirski::protocol
{
    enum class MessageType
    {
        Ping,
        Pong,
        NotificationReceived,
        DisconnectRequested,
        // MediaStateChanged,
        // MediaCommand,
        // TerminalStatus,
        Unknown
    };

    // MessageType messageTypeFromString(const std::string &value)
}
