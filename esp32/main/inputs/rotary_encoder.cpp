// ESP32 rotary encoder GPIO setup and input decoding
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"

#include <cstdint>

#include "rotary_encoder.hpp"
#include "app_constants.hpp"
#include "input.hpp"
#include "haptics.hpp"

namespace swirski::inputs::rotary_encoder
{

    namespace
    {
        constexpr gpio_num_t ROTARY_PIN_A = GPIO_NUM_11;
        constexpr gpio_num_t ROTARY_PIN_B = GPIO_NUM_10;
        constexpr gpio_num_t ROTARY_SWITCH = GPIO_NUM_9;

        constexpr int PCNT_LOW_LIMIT = -32'767;
        constexpr int PCNT_HIGH_LIMIT = 32'767;
        constexpr int PULSES_PER_DETENT = 4;
        constexpr int MAX_PENDING_DETENTS = 4;
        constexpr std::uint32_t PCNT_GLITCH_FILTER_NS = 10'000;

        pcnt_unit_handle_t pcntUnit = nullptr;
        pcnt_channel_handle_t pcntChannelA = nullptr;
        pcnt_channel_handle_t pcntChannelB = nullptr;

        int previousPulseCount = 0;
        int pendingPulseCount = 0;
        int prevSwitch = 0;

        bool dispatchInput(
            swirski::input::input_action action,
            const char *message)
        {
            if (!lvgl_port_lock(20))
            {
                return false;
            }

            ESP_LOGD(swirski::TAG, "%s", message);

            swirski::input::handleInput(action);

            lvgl_port_unlock();

            swirski::hardware::haptics::play(
                swirski::hardware::haptics::Effect::Tick);

            return true;
        }

    }

    void initialise()
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

        pcnt_unit_config_t unitConfig{};
        unitConfig.low_limit = PCNT_LOW_LIMIT;
        unitConfig.high_limit = PCNT_HIGH_LIMIT;

        ESP_ERROR_CHECK(
            pcnt_new_unit(
                &unitConfig,
                &pcntUnit));

        pcnt_glitch_filter_config_t glitchFilterConfig{};
        glitchFilterConfig.max_glitch_ns =
            PCNT_GLITCH_FILTER_NS;

        ESP_ERROR_CHECK(
            pcnt_unit_set_glitch_filter(
                pcntUnit,
                &glitchFilterConfig));

        pcnt_chan_config_t channelAConfig{};
        channelAConfig.edge_gpio_num = ROTARY_PIN_A;
        channelAConfig.level_gpio_num = ROTARY_PIN_B;

        ESP_ERROR_CHECK(
            pcnt_new_channel(
                pcntUnit,
                &channelAConfig,
                &pcntChannelA));

        pcnt_chan_config_t channelBConfig{};
        channelBConfig.edge_gpio_num = ROTARY_PIN_B;
        channelBConfig.level_gpio_num = ROTARY_PIN_A;

        ESP_ERROR_CHECK(
            pcnt_new_channel(
                pcntUnit,
                &channelBConfig,
                &pcntChannelB));

        ESP_ERROR_CHECK(
            pcnt_channel_set_edge_action(
                pcntChannelA,
                PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                PCNT_CHANNEL_EDGE_ACTION_INCREASE));

        ESP_ERROR_CHECK(
            pcnt_channel_set_level_action(
                pcntChannelA,
                PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        ESP_ERROR_CHECK(
            pcnt_channel_set_edge_action(
                pcntChannelB,
                PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                PCNT_CHANNEL_EDGE_ACTION_DECREASE));

        ESP_ERROR_CHECK(
            pcnt_channel_set_level_action(
                pcntChannelB,
                PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

        ESP_ERROR_CHECK(
            pcnt_unit_enable(pcntUnit));

        ESP_ERROR_CHECK(
            pcnt_unit_clear_count(pcntUnit));

        ESP_ERROR_CHECK(
            pcnt_unit_start(pcntUnit));

        previousPulseCount = 0;
        pendingPulseCount = 0;
        prevSwitch = gpio_get_level(ROTARY_SWITCH);

        ESP_LOGI(swirski::TAG, "Rotary encoder initialised");
    }

    void poll()
    {
        int pulseCount = 0;

        ESP_ERROR_CHECK(
            pcnt_unit_get_count(
                pcntUnit,
                &pulseCount));

        int pulseDelta =
            pulseCount -
            previousPulseCount;

        if (pulseDelta > PCNT_HIGH_LIMIT / 2)
        {
            pulseDelta -= PCNT_HIGH_LIMIT;
        }
        else if (pulseDelta < PCNT_LOW_LIMIT / 2)
        {
            pulseDelta -= PCNT_LOW_LIMIT;
        }

        pendingPulseCount += pulseDelta;

        const int maxPendingPulses =
            PULSES_PER_DETENT *
            MAX_PENDING_DETENTS;

        if (pendingPulseCount > maxPendingPulses)
        {
            pendingPulseCount = maxPendingPulses;
        }
        else if (pendingPulseCount < -maxPendingPulses)
        {
            pendingPulseCount = -maxPendingPulses;
        }

        previousPulseCount = pulseCount;

        if (pendingPulseCount >= PULSES_PER_DETENT)
        {
            if (dispatchInput(
                    swirski::input::input_action::Next,
                    "NEXT"))
            {
                pendingPulseCount -= PULSES_PER_DETENT;
            }
        }
        else if (pendingPulseCount <= -PULSES_PER_DETENT)
        {
            if (dispatchInput(
                    swirski::input::input_action::Previous,
                    "PREVIOUS"))
            {
                pendingPulseCount += PULSES_PER_DETENT;
            }
        }

        const int currentSwitch = gpio_get_level(ROTARY_SWITCH);

        // Detect the button changing from not pressed to pressed.
        if (prevSwitch == 1 && currentSwitch == 0)
        {
            dispatchInput(
                swirski::input::input_action::Confirm,
                "CONFIRM");
        }

        prevSwitch = currentSwitch;
    }
}
