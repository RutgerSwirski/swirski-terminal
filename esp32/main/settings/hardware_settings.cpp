#include "hardware_settings.hpp"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_pm.h"

#include "settings_service.hpp"

namespace swirski::settings::hardware
{
    namespace
    {
        constexpr char tag[] = "settings";

        void applyPowerMode(
            swirski::service::settings::PowerMode mode)
        {
            esp_pm_config_t config{};
            config.light_sleep_enable = false;

            switch (mode)
            {
            case swirski::service::settings::PowerMode::Performance:
                config.max_freq_mhz = 160;
                config.min_freq_mhz = 160;
                break;

            case swirski::service::settings::PowerMode::Balanced:
                config.max_freq_mhz = 160;
                config.min_freq_mhz = 80;
                break;

            case swirski::service::settings::PowerMode::Saver:
                config.max_freq_mhz = 80;
                config.min_freq_mhz = 40;
                break;
            }

            const esp_err_t result =
                esp_pm_configure(&config);

            if (result != ESP_OK)
            {
                ESP_LOGE(
                    tag,
                    "Could not apply power mode: %s",
                    esp_err_to_name(result));
            }
        }

    }

    void initialise()
    {
        swirski::service::settings::setPowerModeHandler(
            applyPowerMode);
    }
}
