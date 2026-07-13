
#include "home_screen.hpp"
#include "screen_manager.hpp"
#include "../input/input.hpp"

#include "lvgl.h"

#include <array>

#include <string>

#include <iostream>

namespace swirski::screens::home
{

    namespace
    {
        std::size_t selectedItemIndex = 0;

        std::array<std::string, 4> homeMenuItems{
            "Notifications",
            "Music",
            "Studio",
            "Settings"};

        std::array<lv_obj_t *, 4> menuItemLabels{};

        const std::array<swirski::screens::manager::Screen, 4> homeMenuScreens{
            swirski::screens::manager::Screen::Notifications,
            swirski::screens::manager::Screen::Music,
            swirski::screens::manager::Screen::Studio,
            swirski::screens::manager::Screen::Settings};
    }

    void updateSelection()
    {

        for (std::size_t i = 0; i < homeMenuItems.size(); ++i)
        {
            const bool isSelected = i == selectedItemIndex;

            std::string labelText = isSelected ? std::string("> ") + homeMenuItems[i] : homeMenuItems[i];

            lv_label_set_text(menuItemLabels[i], labelText.c_str());

            const lv_color_t textColor =
                isSelected
                    ? lv_color_hex(0x00ff00)
                    : lv_color_white();

            lv_obj_set_style_text_color(
                menuItemLabels[i],
                textColor,
                LV_PART_MAIN);
        }
    }

    void render()
    {
        // TODO
        lv_obj_t *screenRoot = swirski::screens::manager::createScreenRoot();

        // create a flex container

        lv_obj_t *flexContainer = lv_obj_create(screenRoot);

        lv_obj_set_layout(flexContainer, LV_LAYOUT_FLEX);

        lv_obj_set_flex_flow(flexContainer, LV_FLEX_FLOW_COLUMN);

        lv_obj_set_size(flexContainer, 200, 150);

        lv_obj_set_style_bg_color(flexContainer, lv_color_hex(0x00283d), LV_PART_MAIN);

        lv_obj_set_flex_align(flexContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_set_style_pad_row(flexContainer, 10, LV_PART_MAIN);

        lv_obj_set_style_pad_column(flexContainer, 10, LV_PART_MAIN);

        lv_obj_align(
            flexContainer,
            LV_ALIGN_CENTER,
            0,
            0);

        for (std::size_t i = 0; i < homeMenuItems.size(); ++i)
        {
            menuItemLabels[i] = lv_label_create(flexContainer);
        }

        updateSelection();
    }

    void handleInput(swirski::input::input_action action)
    {

        switch (action)
        {
        case swirski::input::input_action::Previous:
            // only decrement if we are not at the first item, else go back to the last
            if (selectedItemIndex == 0)
            {
                selectedItemIndex = homeMenuItems.size() - 1;
            }
            else
                selectedItemIndex--;

            updateSelection();
            break;
        case swirski::input::input_action::Next:

            // only increment if we are not at the last item, else go back to the first
            if (selectedItemIndex == homeMenuItems.size() - 1)
            {
                selectedItemIndex = 0;
            }
            else
                selectedItemIndex++;

            updateSelection();

            break;
        case swirski::input::input_action::Confirm:

            swirski::screens::manager::showScreen(
                homeMenuScreens[selectedItemIndex]);

            break;
        case swirski::input::input_action::Back:
            break;
        case swirski::input::input_action::Home:
            break;
        }
    }
}