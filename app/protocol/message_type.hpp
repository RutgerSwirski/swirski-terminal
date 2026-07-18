#pragma once

namespace swirski::protocol
{
    enum class MessageType
    {
        Ping,
        Pong,
        TimeSync,
        NotificationReceived,
        NotificationRemoved,
        MusicState,
        WifiScanRequest,
        WifiNetworks,
        WifiConfigure,
        WifiDisconnect,
        WifiStatus,
        WifiInternetTest,
        DisconnectRequested,
        NotificationsSnapshot,
        // MediaStateChanged,
        // MediaCommand,
        // TerminalStatus,
        Unknown
    };

    // MessageType messageTypeFromString(const std::string &value)
}
