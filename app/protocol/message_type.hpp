#pragma once

namespace swirski::protocol
{
    enum class MessageType
    {
        Ping,
        Pong,
        TimeSync,
        NotificationReceived,
        MusicState,
        DisconnectRequested,
        NotificationsSnapshot,
        // MediaStateChanged,
        // MediaCommand,
        // TerminalStatus,
        Unknown
    };

    // MessageType messageTypeFromString(const std::string &value)
}
