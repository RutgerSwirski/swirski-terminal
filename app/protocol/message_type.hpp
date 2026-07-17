#pragma once

namespace swirski::protocol
{
    enum class MessageType
    {
        Ping,
        Pong,
        NotificationReceived,
        DisconnectRequested,
        NotificationsSnapshot,
        // MediaStateChanged,
        // MediaCommand,
        // TerminalStatus,
        Unknown
    };

    // MessageType messageTypeFromString(const std::string &value)
}
