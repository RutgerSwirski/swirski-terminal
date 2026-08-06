#include "device_control.hpp"

#include <utility>

namespace swirski::services::device_control
{
    namespace
    {
        std::function<void()> restartHandler;
        std::function<void()> powerOffHandler;
    }

    void setRestartHandler(std::function<void()> handler)
    {
        restartHandler = std::move(handler);
    }

    void setPowerOffHandler(std::function<void()> handler)
    {
        powerOffHandler = std::move(handler);
    }

    void restart()
    {
        if (restartHandler)
        {
            restartHandler();
        }
    }

    void powerOff()
    {
        if (powerOffHandler)
        {
            powerOffHandler();
        }
    }
}
