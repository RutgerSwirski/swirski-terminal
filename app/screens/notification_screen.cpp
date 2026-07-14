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

        std::cout << "Rendering notification screen for " << notification.title << std::endl;
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