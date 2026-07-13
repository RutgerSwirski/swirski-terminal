
#include <ctime>
#include <iostream>

#include "date_time.hpp"

#include "lvgl.h"

namespace swirski::service::date_time
{

    namespace
    {

        time_t timestamp = 0;

        SyncState syncState = SyncState::NotSynced;

        uint32_t lastUpdateTick = 0;

    }

    time_t getTimestamp()
    {
        return timestamp;
    }

    void initialise(time_t initialTimestamp)
    {
        timestamp = initialTimestamp;
    }

    bool update()
    {

        const uint32_t currentTick = lv_tick_get();

        if (currentTick - lastUpdateTick < 1000)
        {
            return false;
        }

        lastUpdateTick += 1000;
        timestamp += 1;

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

    void setFromTimestamp(std::time_t timestamp)
    {
    }

}