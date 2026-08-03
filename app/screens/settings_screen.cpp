#include "settings_screen.hpp"

#include <array>
#include <ctime>
#include <string>

#include "lvgl.h"

#include "date_time.hpp"
#include "firmware_update.hpp"
#include "ui/keyboard.hpp"
#include "screen_manager.hpp"
#include "settings_service.hpp"
#include "status_bar.hpp"
#include "swirski_ui.hpp"
#include "wifi_service.hpp"

namespace swirski::screens::settings_screen
{
    namespace
    {
        constexpr std::size_t powerModeIndex = 0;
        constexpr std::size_t dateIndex = 1;
        constexpr std::size_t keyboardIndex = 3;
        constexpr std::size_t wifiIndex = 4;
        constexpr std::size_t buildIndex = 5;
        constexpr std::size_t updateIndex = 6;

        std::array<lv_obj_t *, 7> settingLabels{};
        lv_obj_t *updateProgressBar = nullptr;
        std::size_t selectedSettingIndex = 0;
        bool editing = false;
        std::string keyboardText;
        std::uint32_t renderedUpdateRevision = 0;

        void saveKeyboardText(const std::string &text)
        {
            keyboardText = text;
        }

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

        std::string firmwareUpdateText()
        {
            using State =
                swirski::services::firmware_update::State;

            switch (
                swirski::services::firmware_update::getState())
            {
            case State::Downloading:
                return "Updating: " +
                    std::to_string(
                        swirski::services::firmware_update::getProgress()) +
                    "%";
            case State::Restarting:
                return "Update complete - restarting";
            case State::Installed:
                return "Update installed";
            case State::Failed:
                using FailureReason =
                    swirski::services::firmware_update::FailureReason;

                switch (
                    swirski::services::firmware_update::getFailureReason())
                {
                case FailureReason::Start:
                    return "Updater unavailable - retry";
                case FailureReason::Initialisation:
                    return "Couldn't start update - retry";
                case FailureReason::Download:
                    return "Download interrupted - retry";
                case FailureReason::IncompleteImage:
                    return "Firmware download incomplete";
                case FailureReason::InvalidImage:
                    return "Firmware rejected";
                case FailureReason::None:
                    return "Update failed - retry";
                }

                return "Update failed - retry";
            case State::Unavailable:
                return "Update: ESP32 only";
            case State::Idle:
                break;
            }

            return
                swirski::services::wifi_service::getConnectionState() ==
                        swirski::services::wifi_service::ConnectionState::Connected
                    ? "Update firmware"
                    : "Update: connect Wi-Fi";
        }

        void updateScreen()
        {
            const std::array<std::string, 7> settingTexts{
                "Power: " +
                    std::string(
                        powerModeName(
                            swirski::service::settings::getPowerMode())),
                "Date: " + dateText(),
                "Time: " + timeText(),
                "Keyboard: " +
                    (keyboardText.empty() ? "Test" : keyboardText),
                "Wi-Fi: " +
                    (swirski::services::wifi_service::getConnectionState() ==
                            swirski::services::wifi_service::ConnectionState::Connected
                        ? swirski::services::wifi_service::getConnectedSsid()
                        : "Setup"),
                "Build: " +
                    swirski::services::firmware_update::getInstalledBuildId(),
                firmwareUpdateText()};

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

            const bool downloading =
                swirski::services::firmware_update::getState() ==
                swirski::services::firmware_update::State::Downloading;

            lv_bar_set_value(
                updateProgressBar,
                swirski::services::firmware_update::getProgress(),
                LV_ANIM_OFF);

            if (downloading)
            {
                lv_obj_remove_flag(
                    updateProgressBar,
                    LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_add_flag(
                    updateProgressBar,
                    LV_OBJ_FLAG_HIDDEN);
            }

            lv_obj_scroll_to_view(
                downloading && selectedSettingIndex == updateIndex
                    ? updateProgressBar
                    : settingLabels[selectedSettingIndex],
                LV_ANIM_OFF);

            renderedUpdateRevision =
                swirski::services::firmware_update::getRevision();
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

        updateProgressBar =
            lv_bar_create(settingsList);

        lv_obj_set_size(
            updateProgressBar,
            210,
            8);

        lv_bar_set_range(
            updateProgressBar,
            0,
            100);

        lv_obj_set_style_bg_color(
            updateProgressBar,
            swirski::ui::swirski_ui::color::surfaceSoft(),
            LV_PART_MAIN);

        lv_obj_set_style_bg_color(
            updateProgressBar,
            swirski::ui::swirski_ui::color::accentWarm(),
            LV_PART_INDICATOR);

        updateScreen();
    }

    void refreshIfNeeded()
    {
        if (
            renderedUpdateRevision !=
            swirski::services::firmware_update::getRevision())
        {
            updateScreen();
        }
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
            if (!editing && selectedSettingIndex == buildIndex)
            {
                break;
            }

            if (!editing && selectedSettingIndex == updateIndex)
            {
                if (
                    swirski::services::wifi_service::getConnectionState() ==
                    swirski::services::wifi_service::ConnectionState::Connected)
                {
                    swirski::services::firmware_update::start();
                }

                updateScreen();
                break;
            }

            if (!editing && selectedSettingIndex == wifiIndex)
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Wifi);
                break;
            }

            if (!editing && selectedSettingIndex == keyboardIndex)
            {
                swirski::ui::keyboard::open(
                    keyboardText,
                    saveKeyboardText);
                break;
            }

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
