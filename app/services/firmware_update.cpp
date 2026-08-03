#include "firmware_update.hpp"

#include <atomic>

#ifdef ESP_PLATFORM
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

namespace swirski::services::firmware_update
{
    namespace
    {
#ifdef ESP_PLATFORM
        constexpr char firmwareUrl[] =
            "https://github.com/RutgerSwirski/swirski-terminal/"
            "releases/download/firmware-latest/swirski_os_esp32.bin";

        // GitHub redirects release downloads to a long, signed asset URL. The
        // ESP HTTP client's default transmit buffer cannot hold that request URI.
        constexpr int otaHttpTransmitBufferSize = 4096;

        std::atomic<State> state{State::Idle};
#else
        std::atomic<State> state{State::Unavailable};
#endif
        std::atomic_uint8_t progress{0};
        std::atomic_uint32_t revision{0};
        std::atomic<FailureReason> failureReason{
            FailureReason::None};

        void setState(State newState)
        {
            state = newState;
            revision++;
        }

#ifdef ESP_PLATFORM
        void failUpdate(
            FailureReason reason,
            esp_err_t result)
        {
            ESP_LOGE(
                "firmware_update",
                "OTA failed: %s",
                esp_err_to_name(result));

            failureReason = reason;
            setState(State::Failed);
        }

        void updateProgress(esp_https_ota_handle_t handle)
        {
            const int imageSize =
                esp_https_ota_get_image_size(handle);
            const int bytesRead =
                esp_https_ota_get_image_len_read(handle);

            if (imageSize <= 0 || bytesRead < 0)
            {
                return;
            }

            const auto newProgress =
                static_cast<std::uint8_t>(
                    bytesRead * 100 / imageSize);

            if (newProgress != progress.load())
            {
                progress = newProgress;
                revision++;
            }
        }

        void runUpdate(void *)
        {
            esp_http_client_config_t httpConfig{};
            httpConfig.url = firmwareUrl;
            httpConfig.crt_bundle_attach = esp_crt_bundle_attach;
            httpConfig.timeout_ms = 20000;
            httpConfig.keep_alive_enable = true;
            httpConfig.buffer_size_tx = otaHttpTransmitBufferSize;

            esp_https_ota_config_t otaConfig{};
            otaConfig.http_config = &httpConfig;

            esp_https_ota_handle_t handle = nullptr;
            esp_err_t result =
                esp_https_ota_begin(&otaConfig, &handle);

            if (result != ESP_OK)
            {
                failUpdate(
                    FailureReason::Initialisation,
                    result);
                vTaskDelete(nullptr);
                return;
            }

            do
            {
                result = esp_https_ota_perform(handle);
                updateProgress(handle);
            } while (result == ESP_ERR_HTTPS_OTA_IN_PROGRESS);

            if (result != ESP_OK)
            {
                esp_https_ota_abort(handle);
                failUpdate(
                    FailureReason::Download,
                    result);
                vTaskDelete(nullptr);
                return;
            }

            const bool downloadComplete =
                esp_https_ota_is_complete_data_received(handle);

            if (!downloadComplete)
            {
                esp_https_ota_abort(handle);
                failUpdate(
                    FailureReason::IncompleteImage,
                    ESP_ERR_INVALID_SIZE);
                vTaskDelete(nullptr);
                return;
            }

            result = esp_https_ota_finish(handle);

            if (result != ESP_OK)
            {
                failUpdate(
                    FailureReason::InvalidImage,
                    result);
                vTaskDelete(nullptr);
                return;
            }

            progress = 100;
            setState(State::Restarting);
            vTaskDelay(pdMS_TO_TICKS(1500));
            esp_restart();
        }
#endif
    }

    void confirmRunningFirmware()
    {
#ifdef ESP_PLATFORM
        const esp_partition_t *partition =
            esp_ota_get_running_partition();
        esp_ota_img_states_t imageState{};

        if (
            esp_ota_get_state_partition(
                partition,
                &imageState) == ESP_OK &&
            imageState == ESP_OTA_IMG_PENDING_VERIFY)
        {
            if (
                esp_ota_mark_app_valid_cancel_rollback() ==
                ESP_OK)
            {
                progress = 100;
                setState(State::Installed);
            }
        }
#endif
    }

    bool start()
    {
#ifdef ESP_PLATFORM
        const State currentState = state.load();

        if (
            currentState == State::Downloading ||
            currentState == State::Restarting)
        {
            return false;
        }

        progress = 0;
        failureReason = FailureReason::None;
        setState(State::Downloading);

        if (
            xTaskCreate(
                runUpdate,
                "firmware_update",
                8192,
                nullptr,
                3,
                nullptr) != pdPASS)
        {
            failureReason = FailureReason::Start;
            setState(State::Failed);
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    State getState()
    {
        return state.load();
    }

    FailureReason getFailureReason()
    {
        return failureReason.load();
    }

    std::uint8_t getProgress()
    {
        return progress.load();
    }

    std::uint32_t getRevision()
    {
        return revision.load();
    }

    std::string getInstalledBuildId()
    {
#ifdef ESP_PLATFORM
        const esp_app_desc_t *description =
            esp_app_get_description();

        if (description == nullptr)
        {
            return "unknown";
        }

        const std::string version{description->version};
        const std::size_t commitMarker = version.rfind("-g");

        if (commitMarker != std::string::npos)
        {
            return version.substr(commitMarker + 2, 7);
        }

        return version.empty() ? "unknown" : version;
#else
        return "desktop";
#endif
    }
}
