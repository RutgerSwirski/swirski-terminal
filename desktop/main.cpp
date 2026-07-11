#define SDL_MAIN_HANDLED

#include "lvgl.h"
#include "app.hpp"

#include <SDL2/SDL.h>

#include <iostream>

#include "keyboard.hpp"

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

    while (running)
    {

        swirski::inputs::initialiseKeyboard(event, running);

        lv_timer_handler();
        lv_delay_ms(5);
    }

    return 0;
}