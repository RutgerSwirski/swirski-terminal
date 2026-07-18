#include "wifi_screen.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "ui/keyboard.hpp"
#include "lvgl.h"
#include "screen_manager.hpp"
#include "swirski_ui.hpp"
#include "wifi_service.hpp"

namespace swirski::screens::wifi_screen
{
    namespace
    {
        constexpr std::size_t maxVisibleNetworks = 8;

        lv_obj_t *statusLabel = nullptr;
        std::array<lv_obj_t *, maxVisibleNetworks + 1> itemLabels{};
        std::size_t selectedItemIndex = 0;
        std::string selectedSsid;
        std::uint32_t renderedRevision = 0;

        std::string statusText()
        {
            using State =
                swirski::services::wifi_service::ConnectionState;
            using InternetState =
                swirski::services::wifi_service::InternetTestState;

            if (swirski::services::wifi_service::isScanning())
            {
                return "Scanning...";
            }

            if (
                swirski::services::wifi_service::getConnectionState() ==
                State::Connected)
            {
                switch (
                    swirski::services::wifi_service::getInternetTestState())
                {
                case InternetState::Testing:
                    return "Testing swirski.studio...";
                case InternetState::Success:
                    return "Online: " +
                        std::to_string(
                            swirski::services::wifi_service::getInternetLatencyMs()) +
                        " ms";
                case InternetState::Failed:
                    return "Internet test failed";
                case InternetState::Idle:
                    break;
                }
            }

            switch (
                swirski::services::wifi_service::getConnectionState())
            {
            case State::Unavailable:
                return "Wi-Fi unavailable";
            case State::Connecting:
                return "Connecting to " +
                    swirski::services::wifi_service::getConnectedSsid();
            case State::Connected:
                return "Connected: " +
                    swirski::services::wifi_service::getConnectedSsid();
            case State::Failed:
                return "Connection failed";
            case State::Disconnected:
                return "Choose a network";
            }

            return "Choose a network";
        }

        std::size_t itemCount()
        {
            if (
                swirski::services::wifi_service::getConnectionState() ==
                swirski::services::wifi_service::ConnectionState::Connected)
            {
                return 2;
            }

            return std::min(
                maxVisibleNetworks,
                swirski::services::wifi_service::getNetworks().size()) + 1;
        }

        void updateScreen()
        {
            lv_label_set_text(
                statusLabel,
                statusText().c_str());

            const bool wifiConnected =
                swirski::services::wifi_service::getConnectionState() ==
                swirski::services::wifi_service::ConnectionState::Connected;

            lv_label_set_text(
                itemLabels[0],
                wifiConnected
                    ? "Ping swirski.studio"
                    : "Scan again");

            const auto &networks =
                swirski::services::wifi_service::getNetworks();

            for (std::size_t i = 0; i < maxVisibleNetworks; ++i)
            {
                lv_obj_t *label = itemLabels[i + 1];

                if (wifiConnected)
                {
                    if (i == 0)
                    {
                        lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);
                        lv_label_set_text(label, "Disconnect");
                    }
                    else
                    {
                        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
                    }

                    continue;
                }

                if (i >= networks.size())
                {
                    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
                    continue;
                }

                lv_obj_remove_flag(label, LV_OBJ_FLAG_HIDDEN);

                const std::string text =
                    networks[i].ssid +
                    (networks[i].secured ? " *" : "");

                lv_label_set_text(label, text.c_str());
            }

            selectedItemIndex =
                std::min(selectedItemIndex, itemCount() - 1);

            for (std::size_t i = 0; i < itemCount(); ++i)
            {
                swirski::ui::swirski_ui::styleMenuItem(
                    itemLabels[i],
                    i == selectedItemIndex);
            }

            lv_obj_scroll_to_view(
                itemLabels[selectedItemIndex],
                LV_ANIM_OFF);

            renderedRevision =
                swirski::services::wifi_service::getRevision();
        }

        void connectWithPassword(const std::string &password)
        {
            swirski::services::wifi_service::connect(
                selectedSsid,
                password);
        }
    }

    void render()
    {
        lv_obj_t *pageRoot =
            swirski::screens::manager::createPageRoot();

        lv_obj_t *container =
            swirski::ui::swirski_ui::createCard(pageRoot, 175);
        lv_obj_set_width(container, 280);
        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 10);

        swirski::ui::swirski_ui::createBadge(container, "Wi-Fi");

        statusLabel =
            swirski::ui::swirski_ui::createLabel(
                container,
                "",
                swirski::ui::swirski_ui::TextTone::Muted,
                22,
                18);

        lv_obj_t *networkList = lv_obj_create(container);
        lv_obj_remove_style_all(networkList);
        lv_obj_set_pos(networkList, 8, 44);
        lv_obj_set_size(networkList, 235, 105);
        lv_obj_set_layout(networkList, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(networkList, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(
            networkList,
            swirski::ui::swirski_ui::space::xs,
            LV_PART_MAIN);
        lv_obj_set_scroll_dir(networkList, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(networkList, LV_SCROLLBAR_MODE_OFF);

        for (lv_obj_t *&label : itemLabels)
        {
            label = lv_label_create(networkList);
            lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
        }

        updateScreen();

        if (
            swirski::services::wifi_service::getNetworks().empty() &&
            swirski::services::wifi_service::getConnectionState() !=
                swirski::services::wifi_service::ConnectionState::Unavailable)
        {
            swirski::services::wifi_service::scan();
            updateScreen();
        }
    }

    void refreshIfNeeded()
    {
        if (
            renderedRevision !=
            swirski::services::wifi_service::getRevision())
        {
            updateScreen();
        }
    }

    void handleInput(swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            selectedItemIndex = selectedItemIndex == 0
                ? itemCount() - 1
                : selectedItemIndex - 1;
            updateScreen();
            break;

        case swirski::input::input_action::Next:
            selectedItemIndex =
                (selectedItemIndex + 1) % itemCount();
            updateScreen();
            break;

        case swirski::input::input_action::Confirm:
            if (selectedItemIndex == 0)
            {
                if (
                    swirski::services::wifi_service::getConnectionState() ==
                    swirski::services::wifi_service::ConnectionState::Connected)
                {
                    swirski::services::wifi_service::startInternetTest();
                }
                else
                {
                    swirski::services::wifi_service::scan();
                }

                updateScreen();
                break;
            }

            if (
                swirski::services::wifi_service::getConnectionState() ==
                    swirski::services::wifi_service::ConnectionState::Connected &&
                selectedItemIndex == 1)
            {
                swirski::services::wifi_service::disconnect();
                updateScreen();
                break;
            }

            selectedSsid =
                swirski::services::wifi_service::getNetworks()
                    [selectedItemIndex - 1]
                        .ssid;

            if (
                swirski::services::wifi_service::getNetworks()
                    [selectedItemIndex - 1]
                        .secured)
            {
                swirski::ui::keyboard::open(
                    "",
                    connectWithPassword,
                    true);
            }
            else
            {
                swirski::services::wifi_service::connect(
                    selectedSsid,
                    "");
                updateScreen();
            }
            break;

        case swirski::input::input_action::Back:
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Settings);
            break;

        case swirski::input::input_action::Home:
            swirski::screens::manager::showScreen(
                swirski::screens::manager::Screen::Home);
            break;
        }
    }
}
