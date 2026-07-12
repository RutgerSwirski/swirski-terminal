
#include "home_screen.hpp"

#include "screen_manager.hpp"

#include "lvgl.h"

#include <array>

#include <string>

namespace swirski::screens::Home
{

    namespace
    {
        int selectedItemIndex = 0;

        std::array<std::string, 5> homeMenuItems{
            "Home",
            "Notifications",
            "Music",
            "Studio",
            "Settings"};
    }

    void render()
    {
        // TODO
        lv_obj_t *screenRoot = swirski::screens::Manager::createScreenRoot();

        lv_obj_t *title =
            lv_label_create(screenRoot);

        lv_label_set_text(title, "SWIRSKI OS");

        lv_obj_set_style_text_color(
            title,
            lv_color_white(),
            LV_PART_MAIN);

        lv_obj_align(
            title,
            LV_ALIGN_TOP_MID,
            0,
            15);

        // create a flex container

        lv_obj_t *flexContainer = lv_obj_create(screenRoot);

        lv_obj_set_layout(flexContainer, LV_LAYOUT_FLEX);

        lv_obj_set_flex_flow(flexContainer, LV_FLEX_FLOW_COLUMN);

        lv_obj_set_size(flexContainer, 200, 150);

        lv_obj_set_style_bg_color(flexContainer, lv_color_hex(0x00283d), LV_PART_MAIN);

        lv_obj_set_flex_align(flexContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

        lv_obj_set_style_pad_row(flexContainer, 10, LV_PART_MAIN);

        lv_obj_set_style_pad_column(flexContainer, 10, LV_PART_MAIN);

        lv_obj_align(
            flexContainer,
            LV_ALIGN_CENTER,
            0,
            0);

        for (int i = 0; i < homeMenuItems.size(); i++)
        {

            const char *item = homeMenuItems[i].c_str();

            const bool isSelected = i == selectedItemIndex;

            lv_obj_t *menuItem = lv_label_create(flexContainer);

            if (isSelected)
            {

                std::string labelText = std::string("> ") + item;
                lv_label_set_text(menuItem, labelText.c_str());
            }
            else
            {
                lv_label_set_text(menuItem, item);
            }

            lv_obj_set_style_text_color(
                menuItem,
                lv_color_white(),
                LV_PART_MAIN);

            if (i == selectedItemIndex)
            {
                lv_obj_set_style_text_color(

                    menuItem,
                    lv_color_hex(0x00ff00),
                    LV_PART_MAIN);
            }
        }
    }
}