
#include "home_screen.hpp"
#include "screen_manager.hpp"
#include "swirski_ui.hpp"
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

            lv_label_set_text(
                menuItemLabels[i],
                homeMenuItems[i].c_str());

            swirski::ui::swirski_ui::styleMenuItem(
                menuItemLabels[i],
                isSelected);
        }
    }

    void render()
    {
        // TODO
        lv_obj_t *pageRoot = swirski::screens::manager::createPageRoot();

        // create a flex container

        lv_obj_t *flexContainer =
            swirski::ui::swirski_ui::createCard(
                pageRoot,
                150);

        lv_obj_set_width(flexContainer, 230);

        swirski::ui::swirski_ui::createBadge(
            flexContainer,
            "Menu");

        lv_obj_t *menuList =
            lv_obj_create(flexContainer);

        lv_obj_remove_style_all(menuList);

        lv_obj_set_pos(
            menuList,
            10,
            28);

        lv_obj_set_size(
            menuList,
            190,
            105);

        lv_obj_set_layout(menuList, LV_LAYOUT_FLEX);

        lv_obj_set_flex_flow(menuList, LV_FLEX_FLOW_COLUMN);

        lv_obj_set_flex_align(
            menuList,
            LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_START);

        lv_obj_set_style_pad_row(
            menuList,
            swirski::ui::swirski_ui::space::xs,
            LV_PART_MAIN);

        lv_obj_set_style_pad_column(
            menuList,
            swirski::ui::swirski_ui::space::md,
            LV_PART_MAIN);

        lv_obj_align(
            flexContainer,
            LV_ALIGN_TOP_MID,
            0,
            22);

        for (std::size_t i = 0; i < homeMenuItems.size(); ++i)
        {
            menuItemLabels[i] = lv_label_create(menuList);
            lv_label_set_long_mode(
                menuItemLabels[i],
                LV_LABEL_LONG_DOT);
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
