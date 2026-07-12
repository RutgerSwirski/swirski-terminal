#include "app.hpp"

#include "screens/screen_manager.hpp"

namespace swirski::app
{

    void createInterface(lv_display_t *display)
    {
        if (display == nullptr)
        {
            return;
        }

        screens::Manager::initialise(display);

        screens::Manager::showScreen(screens::Manager::Screen::Home);
    }
}
