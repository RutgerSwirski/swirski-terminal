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

        // persist previous A , B and Switch states
        int prevA = 0;
        int prevB = 0;
        int prevSwitch = 0;

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

        prevA = gpio_get_level(ROTARY_PIN_A);
        prevB = gpio_get_level(ROTARY_PIN_B);
        prevSwitch = gpio_get_level(ROTARY_SWITCH);
    }

    void pollRotary()
    {
        const int currentA = gpio_get_level(ROTARY_PIN_A);
        const int currentB = gpio_get_level(ROTARY_PIN_B);
        const int currentSwitch = gpio_get_level(ROTARY_SWITCH);

        // Detect A changing from HIGH to LOW.
        if (prevA == 1 && currentA == 0)
        {
            if (currentB == 1)
            {
                ESP_LOGI(swirski::TAG, "NEXT");

                swirski::input::handleInput(
                    swirski::input::Input::NEXT);
            }
            else
            {
                ESP_LOGI(swirski::TAG, "PREVIOUS");

                swirski::input::handleInput(
                    swirski::input::Input::PREVIOUS);
            }
        }

        // Detect the button changing from not pressed to pressed.
        if (prevSwitch == 1 && currentSwitch == 0)
        {
            ESP_LOGI(swirski::TAG, "CONFIRM");

            swirski::input::handleInput(
                swirski::input::Input::CONFIRM);
        }

        prevA = currentA;
        prevB = currentB;
        prevSwitch = currentSwitch;
    }
}