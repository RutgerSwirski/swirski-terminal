
#include "status_bar.hpp"

#include "lvgl.h"

#include "services/date_time.hpp"
#include "swirski_ui.hpp"

#include <ctime>

#include "system_state.hpp"

namespace swirski::ui::status_bar
{

    namespace
    {
        lv_obj_t *clockLabel = nullptr;
        lv_obj_t *pagePathLabel = nullptr;

        lv_obj_t *connectionLabel = nullptr;
        lv_obj_t *connectionDot = nullptr;

        std::uint32_t lastRenderedSystemRevision = 0;

        bool connected = false;
        bool error = false;

        void renderSystemState(
            const swirski::state::system::SystemStateSnapshot &snapshot)
        {

            if (
                connectionLabel == nullptr ||
                connectionDot == nullptr)
            {
                return;
            }

            const char *transportText = "-";

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
                error = false;
                break;

            case swirski::state::system::ConnectionStatus::Error:
                connected = false;
                error = true;
                break;

            case swirski::state::system::ConnectionStatus::Connecting:
            case swirski::state::system::ConnectionStatus::Disconnected:
                connected = false;
                error = false;
                break;
            }

            if (error)
            {
                lv_obj_set_style_opa(
                    connectionDot,
                    LV_OPA_50,
                    0);

                lv_obj_set_style_bg_color(
                    connectionDot,
                    swirski::ui::swirski_ui::color::accentBright(),
                    LV_PART_MAIN);
            }
            else if (connected)
            {
                lv_obj_set_style_opa(
                    connectionDot,
                    LV_OPA_COVER,
                    0);

                lv_obj_set_style_bg_color(
                    connectionDot,
                    swirski::ui::swirski_ui::color::accent(),
                    LV_PART_MAIN);
            }
            else
            {
                lv_obj_set_style_opa(
                    connectionDot,
                    LV_OPA_COVER,
                    LV_PART_MAIN);

                lv_obj_set_style_bg_color(
                    connectionDot,
                    swirski::ui::swirski_ui::color::accentWarm(),
                    LV_PART_MAIN);
            }
        }

    }

    void create(lv_obj_t *parent)
    {
        lv_obj_set_layout(
            parent,
            LV_LAYOUT_FLEX);

        lv_obj_set_flex_flow(
            parent,
            LV_FLEX_FLOW_ROW);

        lv_obj_set_flex_align(
            parent,
            LV_FLEX_ALIGN_SPACE_BETWEEN,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        lv_obj_set_style_pad_left(
            parent,
            10,
            LV_PART_MAIN);

        lv_obj_set_style_pad_right(
            parent,
            10,
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            parent,
            3,
            LV_PART_MAIN);

        lv_obj_set_style_border_side(
            parent,
            LV_BORDER_SIDE_BOTTOM,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            parent,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);

        lv_obj_t *leftSection = lv_obj_create(parent);
        lv_obj_remove_style_all(leftSection);
        lv_obj_set_size(leftSection, LV_PCT(28), LV_PCT(100));
        lv_obj_set_layout(leftSection, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(leftSection, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(
            leftSection,
            LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        lv_obj_t *centerSection = lv_obj_create(parent);
        lv_obj_remove_style_all(centerSection);
        lv_obj_set_size(centerSection, LV_PCT(54), LV_PCT(100));
        lv_obj_set_layout(centerSection, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(centerSection, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(
            centerSection,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);

        lv_obj_t *rightSection = lv_obj_create(parent);
        lv_obj_remove_style_all(rightSection);
        lv_obj_set_size(rightSection, LV_PCT(18), LV_PCT(100));
        lv_obj_set_layout(rightSection, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(rightSection, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(
            rightSection,
            LV_FLEX_ALIGN_END,
            LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(
            rightSection,
            6,
            LV_PART_MAIN);

        clockLabel = lv_label_create(leftSection);

        const std::time_t timestamp = swirski::service::date_time::getLocalTimestamp();

        std::tm localTime{};

        char clockSnapshot[13] = "-- --- --:--";

        if (gmtime_r(&timestamp, &localTime) != nullptr)
        {
            std::strftime(
                clockSnapshot,
                sizeof(clockSnapshot),
                "%d %b %H:%M",
                &localTime);
        }

        lv_label_set_text(clockLabel, clockSnapshot);

        lv_obj_set_style_text_color(
            clockLabel,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_text_font(
            clockLabel,
            &lv_font_montserrat_12,
            LV_PART_MAIN);

        pagePathLabel = lv_label_create(centerSection);

        lv_label_set_text(pagePathLabel, "SWIRSKI OS");

        lv_obj_set_style_text_color(
            pagePathLabel,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);

        connectionLabel =
            lv_label_create(rightSection);

        lv_label_set_text(
            connectionLabel,
            "-");

        lv_obj_set_style_text_color(
            connectionLabel,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);

        connectionDot =
            lv_obj_create(rightSection);

        lv_obj_set_size(
            connectionDot,
            10,
            10);

        lv_obj_set_style_radius(
            connectionDot,
            LV_RADIUS_CIRCLE,
            0);

        lv_obj_set_style_border_width(
            connectionDot,
            2,
            LV_PART_MAIN);

        lv_obj_set_style_border_color(
            connectionDot,
            swirski::ui::swirski_ui::color::ink(),
            LV_PART_MAIN);

        lv_obj_set_style_pad_all(
            connectionDot,
            0,
            LV_PART_MAIN);

        const auto snapshot =
            swirski::state::system::getSnapshot();

        renderSystemState(snapshot);

        lastRenderedSystemRevision =
            snapshot.revision;
    }

    void updateClock()
    {
        if (clockLabel == nullptr)
        {
            return;
        }

        const std::time_t timestamp =
            swirski::service::date_time::getLocalTimestamp();

        std::tm localTime{};

        if (gmtime_r(&timestamp, &localTime) == nullptr)
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

    void setTitle(const char *title)
    {
        if (pagePathLabel == nullptr)
        {
            return;
        }

        lv_label_set_text(pagePathLabel, title);
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

        renderSystemState(snapshot);

        lastRenderedSystemRevision =
            snapshot.revision;
    }

}
