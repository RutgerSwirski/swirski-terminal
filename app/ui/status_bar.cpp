
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

        const std::string clockSnapshot = swirski::service::date_time::getTimeText();
        lv_label_set_text(clockLabel, clockSnapshot.c_str());

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
            0,
            15);
    }

    void updateClock()
    {
        if (clockLabel == nullptr)
        {
            return;
        }

        const std::string timestamp =
            swirski::service::date_time::getTimeText();

        std::cout << "Timestamp: " << timestamp << std::endl;

        lv_label_set_text(clockLabel, timestamp.c_str());

        // const uint32_t totalMinutes = timestamp / 60;
        // const uint32_t hours = (totalMinutes / 60) % 24;
        // const uint32_t minutes = totalMinutes % 60;

        // char timeText[6];

        // std::snprintf(
        //     timeText,
        //     sizeof(timeText),
        //     "%02lu:%02lu",
        //     static_cast<unsigned long>(hours),
        //     static_cast<unsigned long>(minutes));

        // lv_label_set_text(clockLabel, timeText);
    }
}