// ESP32 rotary encoder GPIO setup and input decoding
#include "driver/gpio.h"
#include "esp_log.h"

#include "rotary_encoder.hpp"
#include "app_constants.hpp"

namespace swirski::inputs
{

    namespace
    {
        constexpr gpio_num_t ROTARY_PIN_A = GPIO_NUM_11;
        constexpr gpio_num_t ROTARY_PIN_B = GPIO_NUM_10;
        constexpr gpio_num_t ROTARY_SWITCH = GPIO_NUM_9;
    }

    void initialiseRotary()
    {
        ESP_LOGI(swirski::TAG, "Initialising rotary encoder");

        gpio_config_t rotaryConfig{};

        rotaryConfig.pin_bit_mask =
            (1ULL << ROTARY_PIN_A) |
            (1ULL << ROTARY_PIN_B) |
            (1ULL << ROTARY_SWITCH);

        rotaryConfig.mode = GPIO_MODE_INPUT;
        rotaryConfig.pull_up_en = GPIO_PULLUP_ENABLE;
        rotaryConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
        rotaryConfig.intr_type = GPIO_INTR_DISABLE;

        ESP_ERROR_CHECK(gpio_config(&rotaryConfig));

        ESP_LOGI(swirski::TAG, "Rotary encoder initialised");
    }

}