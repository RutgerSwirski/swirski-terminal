#pragma once

#include <cstdint>

namespace swirski::service::settings
{
    enum class PowerMode
    {
        Performance,
        Balanced,
        Saver
    };

    using PowerModeHandler = void (*)(PowerMode mode);
    using BrightnessHandler = void (*)(std::uint8_t brightnessPercent);

    constexpr std::uint8_t MINIMUM_BRIGHTNESS_PERCENT = 10;
    constexpr std::uint8_t MAXIMUM_BRIGHTNESS_PERCENT = 100;
    constexpr std::uint8_t BRIGHTNESS_STEP_PERCENT = 10;

    void initialise();
    void setPowerModeHandler(PowerModeHandler handler);
    void setBrightnessHandler(BrightnessHandler handler);

    PowerMode getPowerMode();
    void setPowerMode(PowerMode mode);

    std::uint8_t getBrightnessPercent();
    void setBrightnessPercent(std::uint8_t brightnessPercent);
}
