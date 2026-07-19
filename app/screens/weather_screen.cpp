#include "weather_screen.hpp"

#include <algorithm>
#include <string>

#include "lvgl.h"

#include "screen_manager.hpp"
#include "swirski_ui.hpp"
#include "weather_service.hpp"

namespace swirski::screens::weather_screen
{
    namespace
    {
        int renderedRevision = -1;

        std::string temperatureText(int temperature)
        {
            return std::to_string(temperature) + " C";
        }

        void createForecastRow(
            lv_obj_t *parent,
            const swirski::services::weather_service::ForecastDay &day,
            int y)
        {
            const std::string text =
                day.day + "  " + day.condition + "  " +
                std::to_string(day.lowC) + " / " +
                std::to_string(day.highC) + " C";

            swirski::ui::swirski_ui::createLabel(
                parent,
                text.c_str(),
                swirski::ui::swirski_ui::TextTone::Default,
                y,
                20);
        }
    }

    void render()
    {
        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();
        lv_obj_t *card =
            swirski::ui::swirski_ui::createCard(pageRoot, 176);

        lv_obj_set_width(card, 280);
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 10);
        swirski::ui::swirski_ui::createBadge(card, "Weather");

        if (!swirski::services::weather_service::hasData())
        {
            swirski::ui::swirski_ui::createLabel(
                card,
                "No weather data",
                swirski::ui::swirski_ui::TextTone::Default,
                34,
                22);
            swirski::ui::swirski_ui::createLabel(
                card,
                "Connect your phone to sync",
                swirski::ui::swirski_ui::TextTone::Muted,
                60,
                22);
            renderedRevision =
                swirski::services::weather_service::getRevision();
            return;
        }

        const auto &weather =
            swirski::services::weather_service::getSnapshot();
        const std::string current =
            temperatureText(weather.temperatureC) + "  " +
            weather.condition;

        swirski::ui::swirski_ui::createLabel(
            card,
            current.c_str(),
            swirski::ui::swirski_ui::TextTone::Accent,
            28,
            22);
        swirski::ui::swirski_ui::createLabel(
            card,
            weather.location.c_str(),
            swirski::ui::swirski_ui::TextTone::Muted,
            52,
            18);

        lv_obj_t *divider = lv_obj_create(card);
        lv_obj_remove_style_all(divider);
        lv_obj_set_pos(divider, 0, 76);
        lv_obj_set_size(divider, LV_PCT(100), 2);
        lv_obj_set_style_bg_color(
            divider,
            swirski::ui::swirski_ui::color::accentWarm(),
            LV_PART_MAIN);
        lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);

        const std::size_t rows =
            std::min<std::size_t>(weather.forecastCount, 4);
        for (std::size_t index = 1; index < rows; ++index)
        {
            createForecastRow(
                card,
                weather.forecast[index],
                84 + static_cast<int>(index - 1) * 24);
        }

        renderedRevision =
            swirski::services::weather_service::getRevision();
    }

    void refreshIfNeeded()
    {
        if (
            renderedRevision !=
            swirski::services::weather_service::getRevision())
        {
            render();
        }
    }

    void handleInput(swirski::input::input_action action)
    {
        if (
            action == swirski::input::input_action::Back ||
            action == swirski::input::input_action::Home)
        {
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Home);
        }
    }
}
