#pragma once

#include <cstdint>

namespace swirski::services::firmware_update
{
    enum class State
    {
        Idle,
        Downloading,
        Restarting,
        Installed,
        Failed,
        Unavailable
    };

    enum class FailureReason
    {
        None,
        Start,
        Connection,
        Download,
        IncompleteImage,
        InvalidImage
    };

    void confirmRunningFirmware();
    bool start();

    State getState();
    FailureReason getFailureReason();
    std::uint8_t getProgress();
    std::uint32_t getRevision();
}
