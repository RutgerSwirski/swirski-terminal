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

#ifdef ESP_PLATFORM
        constexpr char NVS_NAMESPACE[] = "settings";
        constexpr char NVS_POWER_MODE_KEY[] = "power_mode";

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
        nvs_close(handle);

        if (
            savedMode >= static_cast<std::int32_t>(PowerMode::Performance) &&
            savedMode <= static_cast<std::int32_t>(PowerMode::Saver))
        {
            powerMode = static_cast<PowerMode>(savedMode);
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
}
