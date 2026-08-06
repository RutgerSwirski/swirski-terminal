#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace swirski::services::firmware_update
{
    enum class State
    {
        Idle,
        Checking,
        Downloading,
        Restarting,
        Installed,
        UpToDate,
        Failed,
        Unavailable
    };

    enum class FailureReason
    {
        None,
        Start,
        Initialisation,
        Download,
        IncompleteImage,
        InvalidImage
    };

    void confirmRunningFirmware();
    void setBeforeRestartHandler(std::function<void()> handler);
    bool start();

    State getState();
    FailureReason getFailureReason();
    std::uint8_t getProgress();
    std::uint32_t getRevision();
    std::string getInstalledBuildId();
}
