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

        constexpr std::int64_t TICK_DURATION_US = 100'000;
        constexpr std::int64_t NOTIFICATION_PULSE_DURATION_US = 100'000;
        constexpr std::int64_t NOTIFICATION_PAUSE_DURATION_US = 75'000;

        enum class NotificationPhase
        {
            FirstPulse,
            Pause,
            SecondPulse
        };

        std::int64_t phaseEndsAtUs = 0;
        bool active = false;

        std::optional<Effect> currentEffect;
        NotificationPhase notificationPhase =
            NotificationPhase::FirstPulse;

        void setMotor(bool enabled)
        {
            ESP_ERROR_CHECK(
                gpio_set_level(
                    HAPTIC_EN,
                    enabled ? 1 : 0));
        }

        void finishEffect()
        {
            setMotor(false);
            active = false;
            currentEffect.reset();
        }
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

        setMotor(false);

        ESP_LOGI(swirski::TAG, "Haptics initialised");
    }

    void play(Effect effect)
    {
        if (active && currentEffect == Effect::Notification)
        {
            // If a notification is already playing, don't interrupt it with a tick
            if (effect == Effect::Tick)
            {
                return;
            }
        }

        // A fast encoder turn must not extend one tick into continuous vibration.
        if (
            active &&
            currentEffect == Effect::Tick &&
            effect == Effect::Tick)
        {
            return;
        }

        const std::int64_t nowUs =
            esp_timer_get_time();

        phaseEndsAtUs =
            nowUs +
            (effect == Effect::Tick
                 ? TICK_DURATION_US
                 : NOTIFICATION_PULSE_DURATION_US);

        active = true;
        currentEffect = effect;

        if (effect == Effect::Notification)
        {
            notificationPhase =
                NotificationPhase::FirstPulse;
        }

        setMotor(true);
    }

    void update()
    {
        if (!active)
        {
            return;
        }

        const std::int64_t nowUs =
            esp_timer_get_time();

        if (nowUs < phaseEndsAtUs)
        {
            return;
        }

        if (currentEffect == Effect::Tick)
        {
            finishEffect();
            return;
        }

        switch (notificationPhase)
        {
        case NotificationPhase::FirstPulse:
            setMotor(false);
            notificationPhase =
                NotificationPhase::Pause;
            phaseEndsAtUs =
                nowUs +
                NOTIFICATION_PAUSE_DURATION_US;
            break;

        case NotificationPhase::Pause:
            setMotor(true);
            notificationPhase =
                NotificationPhase::SecondPulse;
            phaseEndsAtUs =
                nowUs +
                NOTIFICATION_PULSE_DURATION_US;
            break;

        case NotificationPhase::SecondPulse:
            finishEffect();
            break;
        }
    }
}
