#pragma once

namespace swirski::protocol
{
    enum class MessageType
    {
        Ping,
        Pong,
        TimeSync,
        NotificationUpserted,
        NotificationRemoved,
        MusicState,
        WeatherSnapshot,
        WifiScanRequest,
        WifiNetworks,
        WifiConfigure,
        WifiDisconnect,
        WifiStatus,
        WifiInternetTest,
        BatteryStatusRequest,
        BatteryStatus,
        DisconnectRequested,
        NotificationsSnapshot,
        // MediaStateChanged,
        // MediaCommand,
        // TerminalStatus,
        Unknown
    };

    // MessageType messageTypeFromString(const std::string &value)
}
