#include "weather_service.hpp"

#include <cmath>
#include <iostream>
#include <utility>

#include "display_text.hpp"

namespace swirski::services::weather_service
{
    namespace
    {
        WeatherSnapshot currentSnapshot;
        int revision = 0;
    }

    const WeatherSnapshot &getSnapshot()
    {
        return currentSnapshot;
    }

    int getRevision()
    {
        return revision;
    }

    bool hasData()
    {
        return currentSnapshot.forecastCount > 0;
    }

    void setSnapshot(WeatherSnapshot snapshot)
    {
        currentSnapshot = std::move(snapshot);
        revision += 1;
    }

    void handleWeatherSnapshot(JsonObjectConst payload)
    {
        JsonObjectConst current =
            payload["current"].as<JsonObjectConst>();
        JsonArrayConst forecast =
            payload["forecast"].as<JsonArrayConst>();

        if (
            current.isNull() ||
            forecast.isNull() ||
            current["temperatureC"].isNull() ||
            !current["condition"].is<const char *>())
        {
            std::cerr << "Invalid weather.snapshot payload" << std::endl;
            return;
        }

        WeatherSnapshot snapshot;
        snapshot.location = display_text::normalize(
            payload["location"] | "Current location");
        snapshot.condition = display_text::normalize(
            current["condition"].as<std::string>());
        snapshot.temperatureC = static_cast<int>(
            std::lround(current["temperatureC"].as<double>()));
        snapshot.updatedAtMs =
            payload["updatedAtMs"].is<std::uint64_t>()
                ? payload["updatedAtMs"].as<std::uint64_t>()
                : 0;

        for (JsonObjectConst item : forecast)
        {
            if (snapshot.forecastCount == snapshot.forecast.size())
            {
                break;
            }

            if (
                !item["day"].is<const char *>() ||
                !item["condition"].is<const char *>() ||
                item["lowC"].isNull() ||
                item["highC"].isNull())
            {
                continue;
            }

            ForecastDay &day =
                snapshot.forecast[snapshot.forecastCount++];
            day.day = display_text::normalize(
                item["day"].as<std::string>());
            day.condition = display_text::normalize(
                item["condition"].as<std::string>());
            day.lowC = static_cast<int>(
                std::lround(item["lowC"].as<double>()));
            day.highC = static_cast<int>(
                std::lround(item["highC"].as<double>()));
        }

        if (snapshot.forecastCount == 0)
        {
            std::cerr << "Weather snapshot has no forecast days" << std::endl;
            return;
        }

        setSnapshot(std::move(snapshot));
        std::cout << "Applied weather snapshot" << std::endl;
    }
}
