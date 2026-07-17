
#include <ctime>
#include <cstdint>
#include <iostream>

#include "date_time.hpp"

#include "nvs.h"
#include "lvgl.h"

namespace swirski::service::date_time
{

    namespace
    {

        time_t timestamp = 0;

        // SyncState syncState = SyncState::NotSynced;

        uint32_t lastUpdateTick = 0;

        constexpr char NVS_NAMESPACE[] = "datetime";
        constexpr char NVS_TIMESTAMP_KEY[] = "timestamp";

        std::time_t loadCachedTimestamp(
            std::time_t fallbackTimestamp)
        {
            nvs_handle_t handle;

            if (
                nvs_open(
                    NVS_NAMESPACE,
                    NVS_READONLY,
                    &handle) != ESP_OK)
            {
                return fallbackTimestamp;
            }

            std::int64_t cachedTimestamp = 0;

            const esp_err_t result =
                nvs_get_i64(
                    handle,
                    NVS_TIMESTAMP_KEY,
                    &cachedTimestamp);

            nvs_close(handle);

            if (result != ESP_OK)
            {
                return fallbackTimestamp;
            }

            return static_cast<std::time_t>(
                cachedTimestamp);
        }

        void saveCachedTimestamp(
            std::time_t timestamp)
        {
            nvs_handle_t handle;

            if (
                nvs_open(
                    NVS_NAMESPACE,
                    NVS_READWRITE,
                    &handle) != ESP_OK)
            {
                return;
            }

            nvs_set_i64(
                handle,
                NVS_TIMESTAMP_KEY,
                static_cast<std::int64_t>(
                    timestamp));

            nvs_commit(handle);
            nvs_close(handle);
        }

    }

    time_t getTimestamp()
    {
        return timestamp;
    }

    void initialise(time_t initialTimestamp)
    {
        timestamp =
            loadCachedTimestamp(initialTimestamp);

        lastUpdateTick = lv_tick_get();
    }

    bool update()
    {
        const uint32_t currentTick = lv_tick_get();
        const uint32_t elapsedMilliseconds =
            currentTick - lastUpdateTick;

        if (elapsedMilliseconds < 1000)
        {
            return false;
        }

        const uint32_t elapsedSeconds =
            elapsedMilliseconds / 1000;

        lastUpdateTick += elapsedSeconds * 1000;
        timestamp += elapsedSeconds;

        return true;
    }

    std::string getDateText()
    {
        std::tm *utc_time = std::gmtime(&timestamp);

        char buffer[80];

        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", utc_time);

        return buffer;
    }

    std::string getTimeText()
    {

        std::tm *utc_time = std::gmtime(&timestamp);

        char buffer[80];

        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", utc_time);

        std::cout << buffer << std::endl;

        return buffer;
    }

    void setFromTimestamp(std::time_t incomingTimestamp)
    {
        timestamp = incomingTimestamp;
        lastUpdateTick = lv_tick_get();

        saveCachedTimestamp(incomingTimestamp);
    }

}
