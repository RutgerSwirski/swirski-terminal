#include "settings_screen.hpp"

#include <array>
#include <ctime>
#include <string>

#include "lvgl.h"

#include "date_time.hpp"
#include "screen_manager.hpp"
#include "settings_service.hpp"
#include "status_bar.hpp"
#include "swirski_ui.hpp"

namespace swirski::screens::settings_screen
{
    namespace
    {
        constexpr std::size_t powerModeIndex = 0;
        constexpr std::size_t dateIndex = 1;

        std::array<lv_obj_t *, 3> settingLabels{};
        std::size_t selectedSettingIndex = 0;
        bool editing = false;

        const char *powerModeName(
            swirski::service::settings::PowerMode mode)
        {
            switch (mode)
            {
            case swirski::service::settings::PowerMode::Performance:
                return "Performance";
            case swirski::service::settings::PowerMode::Balanced:
                return "Balanced";
            case swirski::service::settings::PowerMode::Saver:
                return "Saver";
            }

            return "Balanced";
        }

        std::tm currentLocalTime()
        {
            const std::time_t timestamp =
                swirski::service::date_time::getLocalTimestamp();

            std::tm localTime{};
            gmtime_r(&timestamp, &localTime);

            return localTime;
        }

        std::string dateText()
        {
            const std::tm localTime = currentLocalTime();
            char text[11]{};

            std::strftime(
                text,
                sizeof(text),
                "%Y-%m-%d",
                &localTime);

            return text;
        }

        std::string timeText()
        {
            const std::tm localTime = currentLocalTime();
            char text[6]{};

            std::strftime(
                text,
                sizeof(text),
                "%H:%M",
                &localTime);

            return text;
        }

        void updateScreen()
        {
            const std::array<std::string, 3> settingTexts{
                "Power: " +
                    std::string(
                        powerModeName(
                            swirski::service::settings::getPowerMode())),
                "Date: " + dateText(),
                "Time: " + timeText()};

            for (std::size_t i = 0; i < settingLabels.size(); ++i)
            {
                lv_label_set_text(
                    settingLabels[i],
                    settingTexts[i].c_str());

                const bool selected =
                    i == selectedSettingIndex;

                swirski::ui::swirski_ui::styleMenuItem(
                    settingLabels[i],
                    selected);

                if (selected && editing)
                {
                    lv_obj_set_style_bg_color(
                        settingLabels[i],
                        swirski::ui::swirski_ui::color::accentBright(),
                        LV_PART_MAIN);
                }
            }

            lv_obj_scroll_to_view(
                settingLabels[selectedSettingIndex],
                LV_ANIM_OFF);
        }

        void changePowerMode(int direction)
        {
            using swirski::service::settings::PowerMode;

            const PowerMode currentMode =
                swirski::service::settings::getPowerMode();

            if (direction > 0)
            {
                swirski::service::settings::setPowerMode(
                    currentMode == PowerMode::Performance
                        ? PowerMode::Balanced
                    : currentMode == PowerMode::Balanced
                        ? PowerMode::Saver
                        : PowerMode::Performance);
            }
            else
            {
                swirski::service::settings::setPowerMode(
                    currentMode == PowerMode::Performance
                        ? PowerMode::Saver
                    : currentMode == PowerMode::Balanced
                        ? PowerMode::Performance
                        : PowerMode::Balanced);
            }
        }

        void changeSelectedSetting(int direction)
        {
            if (selectedSettingIndex == powerModeIndex)
            {
                changePowerMode(direction);
            }
            else
            {
                const std::time_t seconds =
                    selectedSettingIndex == dateIndex
                        ? 24 * 60 * 60
                        : 60;

                swirski::service::date_time::setFromTimestamp(
                    swirski::service::date_time::getTimestamp() +
                    direction * seconds);

                swirski::ui::status_bar::updateClock();
            }

            updateScreen();
        }

        void finishEditing()
        {
            if (selectedSettingIndex != powerModeIndex)
            {
                swirski::service::date_time::save();
            }

            editing = false;
            updateScreen();
        }
    }

    void render()
    {
        editing = false;

        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();

        lv_obj_t *container =
            swirski::ui::swirski_ui::createCard(
                pageRoot,
                165);

        lv_obj_set_width(container, 270);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 15);

        swirski::ui::swirski_ui::createBadge(
            container,
            "Settings");

        lv_obj_t *settingsList = lv_obj_create(container);

        lv_obj_remove_style_all(settingsList);
        lv_obj_set_pos(settingsList, 10, 30);
        lv_obj_set_size(settingsList, 220, 115);
        lv_obj_set_layout(settingsList, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(settingsList, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(
            settingsList,
            swirski::ui::swirski_ui::space::xs,
            LV_PART_MAIN);
        lv_obj_set_scroll_dir(settingsList, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(
            settingsList,
            LV_SCROLLBAR_MODE_OFF);

        for (lv_obj_t *&settingLabel : settingLabels)
        {
            settingLabel = lv_label_create(settingsList);
            lv_label_set_long_mode(
                settingLabel,
                LV_LABEL_LONG_DOT);
        }

        updateScreen();
    }

    void handleInput(
        swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            if (editing)
            {
                changeSelectedSetting(-1);
            }
            else
            {
                selectedSettingIndex =
                    selectedSettingIndex == 0
                        ? settingLabels.size() - 1
                        : selectedSettingIndex - 1;
                updateScreen();
            }
            break;

        case swirski::input::input_action::Next:
            if (editing)
            {
                changeSelectedSetting(1);
            }
            else
            {
                selectedSettingIndex =
                    selectedSettingIndex == settingLabels.size() - 1
                        ? 0
                        : selectedSettingIndex + 1;
                updateScreen();
            }
            break;

        case swirski::input::input_action::Confirm:
            if (editing)
            {
                finishEditing();
            }
            else
            {
                editing = true;
                updateScreen();
            }
            break;

        case swirski::input::input_action::Back:
            if (editing)
            {
                finishEditing();
            }
            else
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Home);
            }
            break;

        case swirski::input::input_action::Home:
            if (editing)
            {
                finishEditing();
            }

            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Home);
            break;
        }
    }
}
