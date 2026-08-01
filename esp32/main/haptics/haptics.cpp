#include "haptics.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "app_constants.hpp"
#include <cstdint>
#include <optional>

#include "esp_timer.h"

namespace swirski::hardware::haptics
{

    namespace
    {
        constexpr gpio_num_t HAPTIC_EN = GPIO_NUM_13;

        // int timerDuration = 0;
        // int currentTimer = 0;

        std::int64_t stopAtUs = 0;
        bool active = false;

        std::optional<Effect> currentEffect;
    }
    void initialise()
    {

        ESP_LOGI(swirski::TAG, "Initialising haptics");

        gpio_config_t hapticsConfig{};

        hapticsConfig.pin_bit_mask =
            1ULL << HAPTIC_EN;

        hapticsConfig.mode = GPIO_MODE_OUTPUT;
        hapticsConfig.pull_up_en = GPIO_PULLUP_DISABLE;
        hapticsConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;

        ESP_ERROR_CHECK(gpio_config(&hapticsConfig));

        ESP_ERROR_CHECK(
            gpio_set_level(HAPTIC_EN, 0));

        ESP_LOGI(swirski::TAG, "Haptics initialised");
    }

    void play(Effect effect)
    {

        if (currentEffect == Effect::Notification)
        {

            // If a notification is already playing, don't interrupt it with a tick
            if (effect == Effect::Tick)
            {
                return;
            }
        }

        const std::int64_t durationUs =
            effect == Effect::Tick
                ? 100'000
                : 150'000;

        stopAtUs =
            esp_timer_get_time() +
            durationUs;

        active = true;
        currentEffect = effect;

        ESP_ERROR_CHECK(
            gpio_set_level(HAPTIC_EN, 1));
    }

    void update()
    {
        if (
            active &&
            esp_timer_get_time() >= stopAtUs)
        {
            ESP_ERROR_CHECK(
                gpio_set_level(HAPTIC_EN, 0));

            active = false;
            currentEffect.reset();
        }
    }
}