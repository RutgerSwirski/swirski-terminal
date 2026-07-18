#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace swirski::services::wifi_service
{
    constexpr std::uint8_t signalBarsForRssi(
        std::int32_t rssi)
    {
        if (rssi >= -55)
        {
            return 3;
        }

        if (rssi >= -70)
        {
            return 2;
        }

        return rssi > -127 ? 1 : 0;
    }

    enum class ConnectionState
    {
        Unavailable,
        Disconnected,
        Connecting,
        Connected,
        Failed
    };

    enum class InternetTestState
    {
        Idle,
        Testing,
        Success,
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
    void disconnect();
    void startInternetTest();
    void connect(
        const std::string &ssid,
        const std::string &password);

    ConnectionState getConnectionState();
    bool isScanning();
    std::int32_t getSignalStrength();
    InternetTestState getInternetTestState();
    std::uint32_t getInternetLatencyMs();
    const std::string &getConnectedSsid();
    const std::vector<Network> &getNetworks();
    std::uint32_t getRevision();
}
