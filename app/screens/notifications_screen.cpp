

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
            lv_obj_t *titleLabel;

            std::string id;
            std::string appName;
            std::string title;
            std::string body;
        };

        std::vector<NotificationRow> notificationRows = {};

        std::size_t selectedNotificationIndex = 0;

    }

    void update()
    {

        for (std::size_t i = 0; i < notificationRows.size(); ++i)
        {

            NotificationRow &row = notificationRows[i];

            const bool isSelected = i == selectedNotificationIndex;

            const std::string labelText = isSelected ? std::string("> ") + row.title : row.title;

            lv_label_set_text(row.titleLabel, labelText.c_str());

            const lv_color_t textColor =
                isSelected
                    ? lv_color_hex(0x00ff00)
                    : lv_color_white();

            lv_obj_set_style_text_color(
                row.titleLabel,
                textColor,
                LV_PART_MAIN);
        }

        if (!notificationRows.empty())
        {
            lv_obj_scroll_to_view(
                notificationRows[selectedNotificationIndex].container,
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

        for (const auto &notification : notifications)
        {

            lv_obj_t *container = lv_obj_create(notificationList);

            lv_obj_set_width(container, LV_PCT(100));
            lv_obj_set_height(container, 55);
            lv_obj_set_flex_grow(container, 0);
            lv_obj_set_style_bg_color(container, lv_color_hex(0x00283d), LV_PART_MAIN);

            lv_obj_t *titleLabel = lv_label_create(container);

            notificationRows.push_back({container, titleLabel, notification.id, notification.appName, notification.title, notification.body});
        }

        update();
    }

    void handleInput(swirski::input::input_action action)
    {

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