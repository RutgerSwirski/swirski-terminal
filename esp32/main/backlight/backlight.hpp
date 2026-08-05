#pragma once

#include <cstdint>

namespace swirski::hardware::backlight
{
    void initialise();

    void setEnabled(bool enabled);
    void setBrightness(std::uint8_t brightnessPercent);
}
