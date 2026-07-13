#pragma once

#include <SDL2/SDL.h>

namespace swirski::inputs::keyboard
{

    void processInput(SDL_Event &event, bool &running);

}