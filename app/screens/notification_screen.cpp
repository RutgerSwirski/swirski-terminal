#include "notification_screen.hpp"

#include <iostream>

#include "input.hpp"

#include "screen_manager.hpp"

#include "notification_service.hpp"
#include "swirski_ui.hpp"

#include "lvgl.h"

namespace swirski::screens::notification_screen
{
    namespace
    {
        lv_obj_t *bodyContainer = nullptr;
        constexpr int BODY_SCROLL_STEP = 28;
    }

    void render(const std::string &notificationId)
    {
        bodyContainer = nullptr;

        const auto notificationResult =
            swirski::services::notification_service::
                getNotificationById(notificationId);

        if (!notificationResult)
        {
            std::cerr
                << "Notification not found: "
                << notificationId
                << std::endl;

            return;
        }

        const auto &notification =
            *notificationResult;

        lv_obj_t *pageRoot =
            swirski::screens::manager::
                createPageRoot();

        lv_obj_t *container =
            lv_obj_create(pageRoot);

        lv_obj_set_size(
            container,
            300,
            190);

        lv_obj_align(
            container,
            LV_ALIGN_TOP_MID,
            0,
            5);

        swirski::ui::swirski_ui::styleCard(container);

        lv_obj_set_style_pad_all(
            container,
            swirski::ui::swirski_ui::space::lg,
            LV_PART_MAIN);

        lv_obj_set_style_pad_row(
            container,
            8,
            LV_PART_MAIN);

        lv_obj_set_flex_flow(
            container,
            LV_FLEX_FLOW_COLUMN);

        lv_obj_set_scrollbar_mode(
            container,
            LV_SCROLLBAR_MODE_OFF);

        lv_obj_clear_flag(
            container,
            LV_OBJ_FLAG_SCROLLABLE);

        const std::string appName =
            notification.appName.empty()
                ? "Unknown app"
                : notification.appName;

        swirski::ui::swirski_ui::createBadge(
            container,
            appName.c_str());

        // Notification title

        lv_obj_t *titleLabel =
            lv_label_create(container);

        const std::string title =
            notification.title.empty()
                ? "New notification"
                : notification.title;

        lv_label_set_text(
            titleLabel,
            title.c_str());

        lv_obj_set_width(
            titleLabel,
            LV_PCT(100));

        lv_label_set_long_mode(
            titleLabel,
            LV_LABEL_LONG_WRAP);

        lv_obj_set_style_text_color(
            titleLabel,
            swirski::ui::swirski_ui::color::text(),
            LV_PART_MAIN);

        // Small separator

        lv_obj_t *separator =
            lv_obj_create(container);

        lv_obj_set_size(
            separator,
            LV_PCT(100),
            1);

        lv_obj_set_style_bg_color(
            separator,
            swirski::ui::swirski_ui::color::accentWarm(),
            LV_PART_MAIN);

        lv_obj_set_style_border_width(
            separator,
            0,
            LV_PART_MAIN);

        lv_obj_clear_flag(
            separator,
            LV_OBJ_FLAG_SCROLLABLE);

        // Notification body

        bodyContainer =
            lv_obj_create(container);

        lv_obj_remove_style_all(
            bodyContainer);

        lv_obj_set_size(
            bodyContainer,
            LV_PCT(100),
            88);

        lv_obj_set_scroll_dir(
            bodyContainer,
            LV_DIR_VER);

        lv_obj_set_scrollbar_mode(
            bodyContainer,
            LV_SCROLLBAR_MODE_AUTO);

        swirski::ui::swirski_ui::styleScrollbar(
            bodyContainer);

        lv_obj_t *bodyLabel =
            lv_label_create(bodyContainer);

        const std::string body =
            notification.body.empty()
                ? "No notification preview available."
                : notification.body;

        lv_label_set_text(
            bodyLabel,
            body.c_str());

        lv_obj_set_width(
            bodyLabel,
            LV_PCT(100));

        lv_label_set_long_mode(
            bodyLabel,
            LV_LABEL_LONG_WRAP);

        lv_obj_set_style_text_color(
            bodyLabel,
            swirski::ui::swirski_ui::color::textMuted(),
            LV_PART_MAIN);
    }

    void handleInput(swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            std::cout << "Previous" << std::endl;

            if (bodyContainer != nullptr)
            {
                lv_obj_scroll_by(
                    bodyContainer,
                    0,
                    BODY_SCROLL_STEP,
                    LV_ANIM_ON);
            }

            break;
        case swirski::input::input_action::Next:
            std::cout << "Next" << std::endl;

            if (bodyContainer != nullptr)
            {
                lv_obj_scroll_by(
                    bodyContainer,
                    0,
                    -BODY_SCROLL_STEP,
                    LV_ANIM_ON);
            }

            break;

        case swirski::input::input_action::Confirm:
            std::cout << "Confirm" << std::endl;
            break;

        case swirski::input::input_action::Back:
            std::cout << "Back from notification" << std::endl;

            // go back to notifications screen
            swirski::screens::manager::showScreen(swirski::screens::manager::Screen::Notifications);
            break;

        case swirski::input::input_action::Home:
            std::cout << "Home" << std::endl;
            break;
        }
    }
}
