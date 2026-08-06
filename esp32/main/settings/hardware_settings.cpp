#include "hardware_settings.hpp"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_pm.h"

#include "backlight.hpp"
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
            const char *modeName = "Balanced";

            switch (mode)
            {
            case swirski::service::settings::PowerMode::Performance:
                modeName = "Performance";
                config.max_freq_mhz = 160;
                config.min_freq_mhz = 160;
                break;

            case swirski::service::settings::PowerMode::Balanced:
                modeName = "Balanced";
                config.max_freq_mhz = 160;
                config.min_freq_mhz = 80;
                break;

            case swirski::service::settings::PowerMode::Saver:
                modeName = "Saver";
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
            else
            {
                ESP_LOGI(
                    tag,
                    "Applied %s power mode: CPU %d-%d MHz, light sleep off",
                    modeName,
                    config.min_freq_mhz,
                    config.max_freq_mhz);
            }
        }

        void applyBrightness(std::uint8_t brightnessPercent)
        {
            swirski::hardware::backlight::setBrightness(
                brightnessPercent);
        }

    }

    void initialise()
    {
        swirski::service::settings::setPowerModeHandler(
            applyPowerMode);
        swirski::service::settings::setBrightnessHandler(
            applyBrightness);
    }
}
