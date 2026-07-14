#define SDL_MAIN_HANDLED

#include "lvgl.h"
#include "app.hpp"

#include <SDL2/SDL.h>

#include <iostream>

#include "keyboard.hpp"

#include "services/date_time.hpp"

#include "ui/status_bar.hpp"

#include <chrono>

#include <ctime>

#include "websocket_transport.hpp"

int main()

{

    SDL_Event event;

    bool running = true;

    lv_init();

    lv_display_t *display =
        lv_sdl_window_create(320, 240);

    lv_sdl_window_set_zoom(display, 2.0f);

    if (display == nullptr)
    {
        return 1;
    }

    lv_display_set_default(display);

    lv_sdl_mouse_create();
    lv_sdl_mousewheel_create();
    lv_sdl_keyboard_create();

    swirski::app::createInterface(display);

    const auto now = std::chrono::system_clock::now();

    const std::time_t currentTime =
        std::chrono::system_clock::to_time_t(now);

    swirski::service::date_time::initialise(currentTime);
    swirski::ui::status_bar::updateClock();

    swirski::transport::websocket::WebSocketTransport websocketTransport;

    // initialise transport websocket
    websocketTransport.initialise();

    while (running)
    {

        websocketTransport.update();

        swirski::inputs::keyboard::processInput(event, running);

        const bool time_changed = swirski::service::date_time::update();

        if (time_changed)
        {
            swirski::ui::status_bar::updateClock();
        }

        lv_timer_handler();
        lv_delay_ms(5);
    }

    return 0;
}