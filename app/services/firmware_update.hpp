#pragma once

#include <cstdint>

namespace swirski::services::firmware_update
{
    enum class State
    {
        Idle,
        Downloading,
        Restarting,
        Failed,
        Unavailable
    };

    void confirmRunningFirmware();
    bool start();

    State getState();
    std::uint8_t getProgress();
    std::uint32_t getRevision();
}
