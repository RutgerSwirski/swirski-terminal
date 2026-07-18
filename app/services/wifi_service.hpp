#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace swirski::services::wifi_service
{
    enum class ConnectionState
    {
        Unavailable,
        Disconnected,
        Connecting,
        Connected,
        Failed
    };

    struct Network
    {
        std::string ssid;
        std::int32_t signalStrength;
        bool secured;
    };

    void initialise();
    void update();
    void scan();
    void connect(
        const std::string &ssid,
        const std::string &password);

    ConnectionState getConnectionState();
    bool isScanning();
    const std::string &getConnectedSsid();
    const std::vector<Network> &getNetworks();
    std::uint32_t getRevision();
}
