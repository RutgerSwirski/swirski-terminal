#include "push_buttons.hpp"

#include "driver/gpio.h"

#include "esp_log.h"

#include "esp_lvgl_port.h"

#include "app_constants.hpp"

namespace swirski::inputs::push_buttons
{
    namespace
    {
        constexpr gpio_num_t BACK_BUTTON = GPIO_NUM_8;

        int prevBack = 0;
    }
    void initialise()
    {
        ESP_LOGI(swirski::TAG, "Initialising push buttons");

        gpio_config_t buttonConfig{};

        buttonConfig.pin_bit_mask =
            (1ULL << BACK_BUTTON);

        buttonConfig.mode = GPIO_MODE_INPUT;
        buttonConfig.pull_up_en = GPIO_PULLUP_ENABLE;
        buttonConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
        buttonConfig.intr_type = GPIO_INTR_DISABLE;

        ESP_ERROR_CHECK(gpio_config(&buttonConfig));

        ESP_LOGI(swirski::TAG, "Push buttons initialised");

        prevBack = gpio_get_level(BACK_BUTTON);
    }

    bool backPressed()
    {
        const int currentBack =
            gpio_get_level(BACK_BUTTON);

        const bool wasPressed =
            prevBack == 1 &&
            currentBack == 0;

        prevBack = currentBack;

        return wasPressed;
    }
}