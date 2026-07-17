
#include <ctime>
#include <iostream>

#include "date_time.hpp"

#include "lvgl.h"

namespace swirski::service::date_time
{

    namespace
    {

        time_t timestamp = 0;

        // SyncState syncState = SyncState::NotSynced;

        uint32_t lastUpdateTick = 0;

    }

    time_t getTimestamp()
    {
        return timestamp;
    }

    void initialise(time_t initialTimestamp)
    {
        timestamp = initialTimestamp;
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
    }

}
