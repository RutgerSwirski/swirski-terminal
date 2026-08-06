#pragma once

#include <functional>

namespace swirski::services::device_control
{
    void setRestartHandler(std::function<void()> handler);
    void setPowerOffHandler(std::function<void()> handler);

    void restart();
    void powerOff();
}
