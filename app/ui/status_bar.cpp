
#include "status_bar.hpp"

#include "lvgl.h"

#include "services/date_time.hpp"

#include <string>

#include <ctime>

#include <iostream>

namespace swirski::ui::status_bar
{
    lv_obj_t *statusBar = nullptr;
    lv_obj_t *clockLabel = nullptr;

    void create(lv_obj_t *parent)
    {
        clockLabel = lv_label_create(parent);

        const std::time_t timestamp = swirski::service::date_time::getTimestamp();

        std::tm localTime{};

        if (localtime_r(&timestamp, &localTime) == nullptr)
        {
            return;
        }

        char clockSnapshot[9];

        std::strftime(
            clockSnapshot,
            sizeof(clockSnapshot),
            "%H:%M:%S",
            &localTime);

        lv_label_set_text(clockLabel, clockSnapshot);

        lv_obj_set_style_text_color(
            clockLabel,
            lv_color_white(),
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            clockLabel,
            &lv_font_montserrat_10,
            LV_PART_MAIN);

        lv_obj_align(
            clockLabel,
            LV_ALIGN_TOP_LEFT,
            10,
            15);
    }

    void updateClock()
    {
        if (clockLabel == nullptr)
        {
            return;
        }

        const std::time_t timestamp =
            swirski::service::date_time::getTimestamp();

        std::tm localTime{};

        if (localtime_r(&timestamp, &localTime) == nullptr)
        {
            return;
        }

        char timeText[9];

        std::strftime(
            timeText,
            sizeof(timeText),
            "%H:%M:%S",
            &localTime);

        lv_label_set_text(clockLabel, timeText);
    }
}