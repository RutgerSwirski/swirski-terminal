#include "backlight.hpp"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include "app_constants.hpp"

#include <cstdint>

namespace swirski::hardware::backlight
{
    namespace
    {
        constexpr gpio_num_t BACKLIGHT_PIN = GPIO_NUM_12;

        // The BC327 high-side stage inverts the ESP32 signal.
        constexpr std::uint32_t BACKLIGHT_ON_LEVEL = 0;
        constexpr std::uint32_t BACKLIGHT_OFF_LEVEL = 1;
    }

    void setEnabled(bool enabled)
    {
        ESP_ERROR_CHECK(
            gpio_set_level(
                BACKLIGHT_PIN,
                enabled
                    ? BACKLIGHT_ON_LEVEL
                    : BACKLIGHT_OFF_LEVEL));
    }

    void initialise()
    {
        ESP_LOGI(
            swirski::TAG,
            "Initialising active-low display backlight on GPIO%d",
            BACKLIGHT_PIN);

        gpio_config_t backlightConfig{};
        backlightConfig.pin_bit_mask =
            1ULL << BACKLIGHT_PIN;
        backlightConfig.mode = GPIO_MODE_OUTPUT;
        backlightConfig.pull_up_en = GPIO_PULLUP_DISABLE;
        backlightConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
        backlightConfig.intr_type = GPIO_INTR_DISABLE;

        ESP_ERROR_CHECK(
            gpio_config(&backlightConfig));

        setEnabled(true);
    }
}
