
#pragma once

#include <cstdint>
#include <optional>

namespace swirski::state::system
{
    enum class TransportType
    {
        None,
        Ble,
        WebSocket
    };

    enum class ConnectionStatus
    {
        Disconnected,
        Connecting,
        Connected,
        Error
    };

    struct ConnectionState
    {
        TransportType transport =
            TransportType::None;

        ConnectionStatus status =
            ConnectionStatus::Disconnected;
    };

    struct SystemStateSnapshot
    {
        ConnectionState connection;

        std::optional<std::uint8_t> batteryPercent;

        bool charging = false;

        std::uint32_t revision = 0;
    };

    SystemStateSnapshot getSnapshot();

    void setConnection(
        TransportType transport,
        ConnectionStatus status);

    void setBatteryPercent(
        std::optional<std::uint8_t> percentage);

    void setCharging(bool charging);

}