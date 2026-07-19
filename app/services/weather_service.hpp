#pragma once

#include <ArduinoJson.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace swirski::services::weather_service
{
    struct ForecastDay
    {
        std::string day;
        std::string condition;
        int lowC = 0;
        int highC = 0;
    };

    struct WeatherSnapshot
    {
        std::string location;
        std::string condition;
        int temperatureC = 0;
        std::uint64_t updatedAtMs = 0;
        std::array<ForecastDay, 4> forecast{};
        std::size_t forecastCount = 0;
    };

    const WeatherSnapshot &getSnapshot();
    int getRevision();
    bool hasData();
    void setSnapshot(WeatherSnapshot snapshot);
    void handleWeatherSnapshot(JsonObjectConst payload);
}
