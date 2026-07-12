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

        screens::manager::initialise(display);

        screens::manager::showScreen(screens::manager::Screen::Home);
    }
}
