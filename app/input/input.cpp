#include "input.hpp"

#include "screen_manager.hpp"

#include "home_screen.hpp"

#include "notifications_screen.hpp"

#include "notification_screen.hpp"

#include "music_screen.hpp"
#include "games_screen.hpp"

#include <iostream>

namespace swirski::input
{

    void handleInput(input_action action)
    {
        const auto currentScreen = swirski::screens::manager::getCurrentScreen();

        switch (currentScreen)
        {
        case swirski::screens::manager::Screen::Home:
            swirski::screens::home::handleInput(action);
            break;
        case swirski::screens::manager::Screen::Notifications:
            swirski::screens::notifications_screen::handleInput(action);
            // handleNotificationsInput(action);
            break;
        case swirski::screens::manager::Screen::Notification:
            swirski::screens::notification_screen::handleInput(action);
            break;
        case swirski::screens::manager::Screen::Music:
            swirski::screens::music_screen::handleInput(action);
            break;
        case swirski::screens::manager::Screen::Games:
            swirski::screens::games_screen::handleInput(action);
            break;
        case swirski::screens::manager::Screen::Studio:
            // handleStudioInput(action);
            break;
        case swirski::screens::manager::Screen::Settings:
            // handleSettingsInput(action);
            break;
        }
    }

}
