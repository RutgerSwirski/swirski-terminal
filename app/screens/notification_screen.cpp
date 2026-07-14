#include "notification_screen.hpp"

#include <iostream>

#include "input.hpp"

#include "screen_manager.hpp"

#include "notifications.hpp"

#include "lvgl.h"

namespace swirski::screens::notification_screen
{

    void render(std::string notificationId)
    {
        // we need to ask the notifications screen what notification was selected

        swirski::services::notifications_service::Notification notification = swirski::services::notifications_service::getNotification(notificationId);

        lv_obj_t *screenRoot = swirski::screens::manager::createScreenRoot();

        lv_obj_t *container = lv_obj_create(screenRoot);

        lv_obj_set_size(container, 300, 200);

        lv_obj_set_style_bg_color(container, lv_color_hex(0x00283d), LV_PART_MAIN);

        lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 35);

        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);

        lv_obj_set_scroll_dir(container, LV_DIR_VER);

        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_AUTO);

        lv_obj_t *titleLabel = lv_label_create(container);

        lv_label_set_text(titleLabel, notification.title.c_str());

        lv_obj_set_style_text_color(titleLabel, lv_color_white(), LV_PART_MAIN);

        lv_obj_t *bodyLabel = lv_label_create(container);

        lv_label_set_text(bodyLabel, notification.body.c_str());

        lv_obj_set_style_text_color(bodyLabel, lv_color_white(), LV_PART_MAIN);
    }

    void handleInput(swirski::input::input_action action)
    {
        switch (action)
        {
        case swirski::input::input_action::Previous:
            std::cout << "Previous" << std::endl;
            break;
        case swirski::input::input_action::Next:
            std::cout << "Next" << std::endl;
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