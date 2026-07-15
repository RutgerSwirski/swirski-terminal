
#include "status_bar.hpp"

#include "lvgl.h"

#include "services/date_time.hpp"

#include <string>

#include <ctime>

#include <iostream>

#include "system_state.hpp"

namespace swirski::ui::status_bar
{
    lv_obj_t *statusBar = nullptr;
    lv_obj_t *clockLabel = nullptr;
    lv_obj_t *pagePathLabel = nullptr;

    lv_obj_t *connectionLabel = nullptr;
    lv_obj_t *connectionDot = nullptr;

    const char *transportText = "-";
    bool connected = false;
    bool error = false;

    std::uint32_t lastRenderedSystemRevision = 0;

    void create(lv_obj_t *parent)
    {
        clockLabel = lv_label_create(parent);

        const std::time_t timestamp = swirski::service::date_time::getTimestamp();

        std::tm localTime{};

        if (localtime_r(&timestamp, &localTime) == nullptr)
        {
            return;
        }

        char clockSnapshot[13]{};

        std::strftime(
            clockSnapshot,
            sizeof(clockSnapshot),
            "%d %b %H:%M",
            &localTime);

        // TODO: create a flex container here

        lv_label_set_text(clockLabel, clockSnapshot);

        lv_obj_set_style_text_color(
            clockLabel,
            lv_color_white(),
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            clockLabel,
            &lv_font_montserrat_12,
            LV_PART_MAIN);

        lv_obj_align(
            clockLabel,
            LV_ALIGN_TOP_LEFT,
            10,
            10);

        // TODO: page indicator component goes here - show what page path is active

        pagePathLabel = lv_label_create(parent);

        lv_label_set_text(pagePathLabel, "SWIRSKI OS");

        lv_obj_set_style_text_color(
            pagePathLabel,
            lv_color_white(),
            LV_PART_MAIN);

        lv_obj_align(
            pagePathLabel,
            LV_ALIGN_TOP_MID,
            10, 10);

        connectionLabel =
            lv_label_create(parent);

        lv_label_set_text(
            connectionLabel,
            "-");

        lv_obj_set_style_text_color(
            connectionLabel,
            lv_color_white(),
            LV_PART_MAIN);

        lv_obj_align(
            connectionLabel,
            LV_ALIGN_TOP_RIGHT,
            -25,
            5);

        connectionDot =
            lv_obj_create(parent);

        lv_obj_set_size(
            connectionDot,
            10,
            10);

        lv_obj_set_style_radius(
            connectionDot,
            LV_RADIUS_CIRCLE,
            0);

        lv_obj_align(
            connectionDot,
            LV_ALIGN_TOP_RIGHT,
            -10,
            10);
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

        char clockSnapshot[13]{};

        std::strftime(
            clockSnapshot,
            sizeof(clockSnapshot),
            "%d %b %H:%M",
            &localTime);

        lv_label_set_text(clockLabel, clockSnapshot);
    }

    void updateSystemState()
    {
        const auto snapshot =
            swirski::state::system::getSnapshot();

        if (
            snapshot.revision ==
            lastRenderedSystemRevision)
        {
            return;
        }

        // Update connection label and dot here.

        switch (snapshot.connection.transport)
        {
        case swirski::state::system::TransportType::Ble:
            transportText = "B";
            break;

        case swirski::state::system::TransportType::WebSocket:
            transportText = "W";
            break;

        case swirski::state::system::TransportType::None:
            transportText = "-";
            break;
        }

        lv_label_set_text(
            connectionLabel,
            transportText);

        switch (snapshot.connection.status)
        {
        case swirski::state::system::ConnectionStatus::Connected:
            connected = true;
            break;

        case swirski::state::system::ConnectionStatus::Error:
            error = true;
            break;

        case swirski::state::system::ConnectionStatus::Connecting:
        case swirski::state::system::ConnectionStatus::Disconnected:
            connected = false;
            break;
        }

        if (connected)
        {
            lv_obj_set_style_opa(
                connectionDot,
                LV_OPA_COVER,
                0);
        }
        else
        {
            lv_obj_set_style_opa(
                connectionDot,
                LV_OPA_30,
                0);
        }

        lastRenderedSystemRevision =
            snapshot.revision;
    }
}