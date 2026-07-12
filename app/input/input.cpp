#include "input.hpp"

#include "screens/screen_manager.hpp"

#include "screens/home_screen.hpp"

#include <iostream>

namespace swirski::input
{

    void handleInput(InputAction action)
    {
        const auto currentScreen = swirski::screens::Manager::getCurrentScreen();

        switch (currentScreen)
        {
        case swirski::screens::Manager::Screen::Home:
            swirski::screens::Home::handleInput(action);
            break;
        case swirski::screens::Manager::Screen::Notifications:
            // handleNotificationsInput(action);
            break;
        case swirski::screens::Manager::Screen::Music:
            // handleMusicInput(action);
            break;
        case swirski::screens::Manager::Screen::Studio:
            // handleStudioInput(action);
            break;
        case swirski::screens::Manager::Screen::Settings:
            // handleSettingsInput(action);
            break;
        }
    }

}