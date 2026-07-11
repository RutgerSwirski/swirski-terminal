#include "app.hpp"

#include "screen/screen.hpp"

namespace swirski::app
{

    void createInterface(lv_display_t *display)
    {
        if (display == nullptr)
        {
            return;
        }

        screen::initialise(display);

        screen::showScreen(screen::Screen::Home);
    }
}
