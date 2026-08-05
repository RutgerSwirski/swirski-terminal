#include "settings_service.hpp"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "nvs.h"
#endif

namespace swirski::service::settings
{
    namespace
    {
        PowerMode powerMode = PowerMode::Balanced;
        PowerModeHandler powerModeHandler = nullptr;
        std::uint8_t brightnessPercent = MAXIMUM_BRIGHTNESS_PERCENT;
        BrightnessHandler brightnessHandler = nullptr;

#ifdef ESP_PLATFORM
        constexpr char NVS_NAMESPACE[] = "settings";
        constexpr char NVS_POWER_MODE_KEY[] = "power_mode";
        constexpr char NVS_BRIGHTNESS_KEY[] = "brightness";

        void savePowerMode()
        {
            nvs_handle_t handle;

            if (
                nvs_open(
                    NVS_NAMESPACE,
                    NVS_READWRITE,
                    &handle) != ESP_OK)
            {
                return;
            }

            nvs_set_i32(
                handle,
                NVS_POWER_MODE_KEY,
                static_cast<std::int32_t>(powerMode));
            nvs_commit(handle);
            nvs_close(handle);
        }

        void saveBrightness()
        {
            nvs_handle_t handle;

            if (
                nvs_open(
                    NVS_NAMESPACE,
                    NVS_READWRITE,
                    &handle) != ESP_OK)
            {
                return;
            }

            nvs_set_u8(
                handle,
                NVS_BRIGHTNESS_KEY,
                brightnessPercent);
            nvs_commit(handle);
            nvs_close(handle);
        }
#endif
    }

    void initialise()
    {
#ifdef ESP_PLATFORM
        nvs_handle_t handle;

        if (
            nvs_open(
                NVS_NAMESPACE,
                NVS_READONLY,
                &handle) != ESP_OK)
        {
            return;
        }

        std::int32_t savedMode =
            static_cast<std::int32_t>(PowerMode::Balanced);

        nvs_get_i32(
            handle,
            NVS_POWER_MODE_KEY,
            &savedMode);

        if (
            savedMode >= static_cast<std::int32_t>(PowerMode::Performance) &&
            savedMode <= static_cast<std::int32_t>(PowerMode::Saver))
        {
            powerMode = static_cast<PowerMode>(savedMode);
        }

        std::uint8_t savedBrightness = MAXIMUM_BRIGHTNESS_PERCENT;

        nvs_get_u8(
            handle,
            NVS_BRIGHTNESS_KEY,
            &savedBrightness);
        nvs_close(handle);

        if (
            savedBrightness >= MINIMUM_BRIGHTNESS_PERCENT &&
            savedBrightness <= MAXIMUM_BRIGHTNESS_PERCENT &&
            savedBrightness % BRIGHTNESS_STEP_PERCENT == 0)
        {
            brightnessPercent = savedBrightness;
        }
#endif
    }

    void setPowerModeHandler(PowerModeHandler handler)
    {
        powerModeHandler = handler;

        if (powerModeHandler != nullptr)
        {
            powerModeHandler(powerMode);
        }
    }

    void setBrightnessHandler(BrightnessHandler handler)
    {
        brightnessHandler = handler;

        if (brightnessHandler != nullptr)
        {
            brightnessHandler(brightnessPercent);
        }
    }

    PowerMode getPowerMode()
    {
        return powerMode;
    }

    void setPowerMode(PowerMode newMode)
    {
        if (powerMode == newMode)
        {
            return;
        }

        powerMode = newMode;

#ifdef ESP_PLATFORM
        savePowerMode();
#endif

        if (powerModeHandler != nullptr)
        {
            powerModeHandler(powerMode);
        }
    }

    std::uint8_t getBrightnessPercent()
    {
        return brightnessPercent;
    }

    void setBrightnessPercent(std::uint8_t newBrightnessPercent)
    {
        if (newBrightnessPercent < MINIMUM_BRIGHTNESS_PERCENT)
        {
            newBrightnessPercent = MINIMUM_BRIGHTNESS_PERCENT;
        }
        else if (newBrightnessPercent > MAXIMUM_BRIGHTNESS_PERCENT)
        {
            newBrightnessPercent = MAXIMUM_BRIGHTNESS_PERCENT;
        }

        newBrightnessPercent =
            static_cast<std::uint8_t>(
                (newBrightnessPercent / BRIGHTNESS_STEP_PERCENT) *
                BRIGHTNESS_STEP_PERCENT);

        if (brightnessPercent == newBrightnessPercent)
        {
            return;
        }

        brightnessPercent = newBrightnessPercent;

#ifdef ESP_PLATFORM
        saveBrightness();
#endif

        if (brightnessHandler != nullptr)
        {
            brightnessHandler(brightnessPercent);
        }
    }
}
