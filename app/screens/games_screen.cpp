#include "games_screen.hpp"

#include <array>
#include <cstddef>
#include <string>

#include "lvgl.h"

#include "screen_manager.hpp"
#include "swirski_ui.hpp"

namespace swirski::screens::games_screen
{
    namespace
    {
        std::size_t selectedGameIndex = 0;

        std::array<std::string, 4> gameItems{
            "Pong",
            "Blackjack",
            "Snake",
            "Tetris"};

        std::array<lv_obj_t *, 4> gameItemLabels{};

        void updateSelection()
        {
            for (std::size_t i = 0; i < gameItems.size(); ++i)
            {
                lv_label_set_text(
                    gameItemLabels[i],
                    gameItems[i].c_str());

                swirski::ui::swirski_ui::styleMenuItem(
                    gameItemLabels[i],
                    i == selectedGameIndex);
            }

            lv_obj_scroll_to_view(
                gameItemLabels[selectedGameIndex],
                LV_ANIM_OFF);
        }
    }

    void render()
    {
        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();

        lv_obj_t *container =
            swirski::ui::swirski_ui::createCard(
                pageRoot,
                150);

        lv_obj_set_width(
            container,
            230);

        lv_obj_align(
            container,
            LV_ALIGN_TOP_MID,
            0,
            22);

        swirski::ui::swirski_ui::createBadge(
            container,
            "Games");

        swirski::ui::swirski_ui::createLabel(
            container,
            "Choose a game",
            swirski::ui::swirski_ui::TextTone::Muted,
            24,
            18);

        lv_obj_t *gameList =
            lv_obj_create(container);

        lv_obj_remove_style_all(
            gameList);

        lv_obj_set_pos(
            gameList,
            10,
            48);

        lv_obj_set_size(
            gameList,
            190,
            82);

        lv_obj_set_scroll_dir(
            gameList,
            LV_DIR_VER);

        lv_obj_set_scrollbar_mode(
            gameList,
            LV_SCROLLBAR_MODE_OFF);

        lv_obj_set_layout(
            gameList,
            LV_LAYOUT_FLEX);

        lv_obj_set_flex_flow(
            gameList,
            LV_FLEX_FLOW_COLUMN);

        lv_obj_set_flex_align(
            gameList,
            LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_START);

        lv_obj_set_style_pad_row(
            gameList,
            swirski::ui::swirski_ui::space::xs,
            LV_PART_MAIN);

        for (std::size_t i = 0; i < gameItems.size(); ++i)
        {
            gameItemLabels[i] =
                lv_label_create(gameList);

            lv_label_set_long_mode(
                gameItemLabels[i],
                LV_LABEL_LONG_DOT);
        }

        updateSelection();
    }

    void handleInput(
        swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            if (selectedGameIndex == 0)
            {
                selectedGameIndex =
                    gameItems.size() - 1;
            }
            else
            {
                selectedGameIndex--;
            }

            updateSelection();
            break;

        case swirski::input::input_action::Next:
            if (selectedGameIndex == gameItems.size() - 1)
            {
                selectedGameIndex = 0;
            }
            else
            {
                selectedGameIndex++;
            }

            updateSelection();
            break;

        case swirski::input::input_action::Back:
        case swirski::input::input_action::Home:
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Home);
            break;

        case swirski::input::input_action::Confirm:
            if (selectedGameIndex == 0)
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Pong);
            }
            else if (selectedGameIndex == 1)
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Blackjack);
            }
            else if (selectedGameIndex == 2)
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Snake);
            }
            else if (selectedGameIndex == 3)
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Tetris);
            }
            break;
        }
    }
}
