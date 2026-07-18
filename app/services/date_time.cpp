
#include <ctime>
#include <cstdint>
#include <iostream>

#include "date_time.hpp"

#include "lvgl.h"

#ifdef ESP_PLATFORM
#include "nvs.h"
#endif

namespace swirski::service::date_time
{

    namespace
    {

        time_t timestamp = 0;
        int timezoneOffsetMinutes = 0;

        // SyncState syncState = SyncState::NotSynced;

        uint32_t lastUpdateTick = 0;

#ifdef ESP_PLATFORM
        constexpr char NVS_NAMESPACE[] = "datetime";
        constexpr char NVS_TIMESTAMP_KEY[] = "timestamp";
        constexpr char NVS_TIMEZONE_KEY[] = "timezone";
#endif

        std::time_t loadCachedTimestamp(
            std::time_t fallbackTimestamp)
        {
#ifndef ESP_PLATFORM
            return fallbackTimestamp;
#else
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
#endif
        }

        int loadCachedTimezoneOffset()
        {
#ifndef ESP_PLATFORM
            return 0;
#else
            nvs_handle_t handle;

            if (
                nvs_open(
                    NVS_NAMESPACE,
                    NVS_READONLY,
                    &handle) != ESP_OK)
            {
                return 0;
            }

            std::int32_t cachedOffset = 0;

            nvs_get_i32(
                handle,
                NVS_TIMEZONE_KEY,
                &cachedOffset);

            nvs_close(handle);
            return static_cast<int>(cachedOffset);
#endif
        }

        void saveCachedTime()
        {
#ifndef ESP_PLATFORM
            (void)timestamp;
#else
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

            nvs_set_i32(
                handle,
                NVS_TIMEZONE_KEY,
                static_cast<std::int32_t>(
                    timezoneOffsetMinutes));

            nvs_commit(handle);
            nvs_close(handle);
#endif
        }

    }

    time_t getTimestamp()
    {
        return timestamp;
    }

    time_t getLocalTimestamp()
    {
        return timestamp +
               static_cast<std::time_t>(timezoneOffsetMinutes) * 60;
    }

    void initialise(time_t initialTimestamp)
    {
        timestamp =
            loadCachedTimestamp(initialTimestamp);

        timezoneOffsetMinutes =
            loadCachedTimezoneOffset();

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
        const std::time_t localTimestamp =
            getLocalTimestamp();

        std::tm *utc_time = std::gmtime(&localTimestamp);

        char buffer[80];

        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", utc_time);

        return buffer;
    }

    std::string getTimeText()
    {

        const std::time_t localTimestamp =
            getLocalTimestamp();

        std::tm *utc_time = std::gmtime(&localTimestamp);

        char buffer[80];

        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", utc_time);

        std::cout << buffer << std::endl;

        return buffer;
    }

    void setFromTimestamp(std::time_t incomingTimestamp)
    {
        timestamp = incomingTimestamp;
        lastUpdateTick = lv_tick_get();
    }

    void setFromPhoneTime(
        std::time_t utcTimestamp,
        int incomingTimezoneOffsetMinutes)
    {
        timestamp = utcTimestamp;
        timezoneOffsetMinutes =
            incomingTimezoneOffsetMinutes;
        lastUpdateTick = lv_tick_get();
    }

    void save()
    {
        saveCachedTime();
    }

}
