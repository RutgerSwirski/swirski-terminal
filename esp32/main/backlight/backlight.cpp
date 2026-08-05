#include "backlight.hpp"

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

#include "app_constants.hpp"

#include <cstdint>

namespace swirski::hardware::backlight
{
    namespace
    {
        constexpr int BACKLIGHT_PIN = 12;
        constexpr ledc_mode_t PWM_SPEED_MODE = LEDC_LOW_SPEED_MODE;
        constexpr ledc_timer_t PWM_TIMER = LEDC_TIMER_0;
        constexpr ledc_channel_t PWM_CHANNEL = LEDC_CHANNEL_0;
        constexpr ledc_timer_bit_t PWM_RESOLUTION = LEDC_TIMER_10_BIT;
        constexpr std::uint32_t PWM_FREQUENCY_HZ = 5000;
        constexpr std::uint32_t MAXIMUM_DUTY = (1U << 10) - 1;

        bool initialised = false;
        bool enabled = true;
        std::uint8_t brightnessPercent = 100;

        void applyOutput()
        {
            if (!initialised)
            {
                return;
            }

            const std::uint32_t duty =
                enabled
                    ? MAXIMUM_DUTY * brightnessPercent / 100
                    : 0;

            ESP_ERROR_CHECK(
                ledc_set_duty(
                    PWM_SPEED_MODE,
                    PWM_CHANNEL,
                    duty));

            ESP_ERROR_CHECK(
                ledc_update_duty(
                    PWM_SPEED_MODE,
                    PWM_CHANNEL));
        }
    }

    void setEnabled(bool newEnabled)
    {
        enabled = newEnabled;
        applyOutput();
    }

    void setBrightness(std::uint8_t newBrightnessPercent)
    {
        brightnessPercent =
            newBrightnessPercent > 100
                ? 100
                : newBrightnessPercent;
        applyOutput();
    }

    void initialise()
    {
        ESP_LOGI(
            swirski::TAG,
            "Initialising inverted backlight PWM on GPIO%d",
            BACKLIGHT_PIN);

        ledc_timer_config_t timerConfig{};
        timerConfig.speed_mode = PWM_SPEED_MODE;
        timerConfig.duty_resolution = PWM_RESOLUTION;
        timerConfig.timer_num = PWM_TIMER;
        timerConfig.freq_hz = PWM_FREQUENCY_HZ;
        timerConfig.clk_cfg = LEDC_AUTO_CLK;

        ESP_ERROR_CHECK(
            ledc_timer_config(&timerConfig));

        ledc_channel_config_t channelConfig{};
        channelConfig.gpio_num = BACKLIGHT_PIN;
        channelConfig.speed_mode = PWM_SPEED_MODE;
        channelConfig.channel = PWM_CHANNEL;
        channelConfig.timer_sel = PWM_TIMER;
        channelConfig.duty = MAXIMUM_DUTY;
        channelConfig.hpoint = 0;
        channelConfig.flags.output_invert = 1;

        ESP_ERROR_CHECK(
            ledc_channel_config(&channelConfig));

        initialised = true;
        applyOutput();
    }
}
