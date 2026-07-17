

#include "input.hpp"

#include "screen_manager.hpp"
#include "notifications_screen.hpp"
#include "notification_screen.hpp"
#include "notification_service.hpp"

#include <vector>

#include <iostream>

#include <string>

#include "lvgl.h"

namespace swirski::screens::notifications_screen
{

    namespace
    {

        struct NotificationRow
        {
            lv_obj_t *container;
            lv_obj_t *appNameLabel;
            lv_obj_t *titleLabel;
            lv_obj_t *bodyLabel;

            std::string id;
            std::string appName;
            std::string title;
            std::string body;
        };

        std::vector<NotificationRow> notificationRows = {};

        std::size_t selectedNotificationIndex = 0;

        int renderedNotificationRevision = -1;

        constexpr std::size_t MAX_RENDERED_NOTIFICATIONS = 20;

    }

    void update()
    {
        for (
            std::size_t i = 0;
            i < notificationRows.size();
            ++i)
        {
            NotificationRow &row =
                notificationRows[i];

            const bool isSelected =
                i == selectedNotificationIndex;

            const std::string titleText =
                isSelected
                    ? std::string("> ") + row.title
                    : row.title;

            lv_label_set_text(
                row.titleLabel,
                titleText.c_str());

            const lv_color_t titleColor =
                isSelected
                    ? lv_color_hex(0x00FF00)
                    : lv_color_white();

            const lv_color_t appNameColor =
                isSelected
                    ? lv_color_hex(0x00CC88)
                    : lv_color_hex(0x8FAAB7);

            const lv_color_t bodyColor =
                isSelected
                    ? lv_color_hex(0xD5E8E0)
                    : lv_color_hex(0x8FAAB7);

            lv_obj_set_style_text_color(
                row.titleLabel,
                titleColor,
                LV_PART_MAIN);

            lv_obj_set_style_text_color(
                row.appNameLabel,
                appNameColor,
                LV_PART_MAIN);

            lv_obj_set_style_text_color(
                row.bodyLabel,
                bodyColor,
                LV_PART_MAIN);
        }

        if (!notificationRows.empty())
        {
            lv_obj_scroll_to_view(
                notificationRows[selectedNotificationIndex]
                    .container,
                LV_ANIM_OFF);
        }
    }

    void render()
    {

        notificationRows.clear();

        std::vector<swirski::services::notification_service::Notification> notifications = swirski::services::notification_service::getNotifications();

        std::cout << "Rendering notifications screen" << std::endl;

        lv_obj_t *pageRoot = swirski::screens::manager::createPageRoot();

        lv_obj_t *notificationList = lv_obj_create(pageRoot);

        lv_obj_set_size(notificationList, 300, 200);

        lv_obj_set_style_bg_color(notificationList, lv_color_hex(0x00283d), LV_PART_MAIN);

        lv_obj_align(notificationList, LV_ALIGN_TOP_MID, 0, 5);

        lv_obj_set_flex_flow(notificationList, LV_FLEX_FLOW_COLUMN);

        lv_obj_set_scroll_dir(notificationList, LV_DIR_VER);

        lv_obj_set_scrollbar_mode(notificationList, LV_SCROLLBAR_MODE_AUTO);

        // Remove the list's internal padding.
        lv_obj_set_style_pad_all(
            notificationList,
            0,
            LV_PART_MAIN);

        lv_obj_set_style_pad_row(
            notificationList,
            4,
            LV_PART_MAIN);

        // remove container border
        lv_obj_set_style_border_width(
            notificationList,
            0,
            LV_PART_MAIN);

        for (const auto &notification : notifications)
        {
            if (
                notificationRows.size() >=
                MAX_RENDERED_NOTIFICATIONS)
            {
                break;
            }

            const std::string rowTitle =
                !notification.title.empty()
                    ? notification.title
                : !notification.body.empty()
                    ? notification.body
                    : "New notification";

            const std::string bodyPreview =
                notification.body.empty()
                    ? "No preview available"
                    : notification.body;

            lv_obj_t *container =
                lv_obj_create(notificationList);

            lv_obj_set_width(
                container,
                LV_PCT(100));

            lv_obj_set_height(
                container,
                70);

            lv_obj_set_flex_grow(
                container,
                0);

            lv_obj_set_style_bg_color(
                container,
                lv_color_hex(0x00283D),
                LV_PART_MAIN);

            // lv_obj_set_style_border_width(
            //     container,
            //     0,
            //     LV_PART_MAIN);

            lv_obj_set_style_pad_top(
                container,
                3,
                LV_PART_MAIN);

            lv_obj_set_style_pad_left(
                container,
                6,
                LV_PART_MAIN);

            lv_obj_set_scrollbar_mode(
                container,
                LV_SCROLLBAR_MODE_OFF);

            lv_obj_clear_flag(
                container,
                LV_OBJ_FLAG_SCROLLABLE);

            // App name

            lv_obj_t *appNameLabel =
                lv_label_create(container);

            lv_label_set_text(
                appNameLabel,
                notification.appName.c_str());

            lv_obj_set_size(
                appNameLabel,
                LV_PCT(100),
                16);

            lv_label_set_long_mode(
                appNameLabel,
                LV_LABEL_LONG_DOT);

            lv_obj_align(
                appNameLabel,
                LV_ALIGN_TOP_LEFT,
                0,
                0);

            // Title

            lv_obj_t *titleLabel =
                lv_label_create(container);

            lv_label_set_text(
                titleLabel,
                rowTitle.c_str());

            lv_obj_set_size(
                titleLabel,
                LV_PCT(100),
                18);

            lv_label_set_long_mode(
                titleLabel,
                LV_LABEL_LONG_DOT);

            lv_obj_align(
                titleLabel,
                LV_ALIGN_TOP_LEFT,
                0,
                18);

            // Body

            lv_obj_t *bodyLabel =
                lv_label_create(container);

            lv_label_set_text(
                bodyLabel,
                bodyPreview.c_str());

            lv_obj_set_size(
                bodyLabel,
                LV_PCT(100),
                18);

            lv_label_set_long_mode(
                bodyLabel,
                LV_LABEL_LONG_DOT);

            lv_obj_align(
                bodyLabel,
                LV_ALIGN_TOP_LEFT,
                0,
                38);

            notificationRows.push_back({
                container,
                appNameLabel,
                titleLabel,
                bodyLabel,
                notification.id,
                notification.appName,
                rowTitle,
                bodyPreview,
            });
        }

        if (notificationRows.empty())
        {
            selectedNotificationIndex = 0;
        }
        else if (selectedNotificationIndex >= notificationRows.size())
        {
            selectedNotificationIndex =
                notificationRows.size() - 1;
        }

        renderedNotificationRevision =
            swirski::services::notification_service::
                revision;

        update();
    }

    void refreshIfNeeded()
    {
        if (
            renderedNotificationRevision ==
            swirski::services::notification_service::
                revision)
        {
            return;
        }

        render();
    }

    void handleInput(swirski::input::input_action action)
    {

        if (notificationRows.empty())
        {
            if (
                action ==
                swirski::input::input_action::Back)
            {
                swirski::screens::manager::showScreen(
                    swirski::screens::manager::Screen::Home);
            }

            return;
        }

        switch (action)
        {

        case swirski::input::input_action::Previous:
            std::cout << "Previous" << std::endl;
            if (selectedNotificationIndex == 0)
            {
                selectedNotificationIndex = notificationRows.size() - 1;
            }
            else
                selectedNotificationIndex--;

            update();

            break;
        case swirski::input::input_action::Next:
            std::cout << "Next" << std::endl;
            if (selectedNotificationIndex == notificationRows.size() - 1)
            {
                selectedNotificationIndex = 0;
            }
            else
                selectedNotificationIndex++;

            update();

            break;
        case swirski::input::input_action::Confirm:
            std::cout << "Confirm" << std::endl;

            swirski::screens::manager::showNotificationScreen(notificationRows[selectedNotificationIndex].id);

            break;
        case swirski::input::input_action::Back:
            std::cout << "Back" << std::endl;

            swirski::screens::manager::showScreen(swirski::screens::manager::Screen::Home);
            break;
        case swirski::input::input_action::Home:
            break;
        }
    }
}
