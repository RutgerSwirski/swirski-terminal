#include "input.hpp"

#include "screens/screen_manager.hpp"

#include "screens/home_screen.hpp"

#include "screens/notifications_screen.hpp"

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
        case swirski::screens::manager::Screen::Music:
            // handleMusicInput(action);
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