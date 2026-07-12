#include "keyboard.hpp"

#include <iostream>

#include "input.hpp"

#include <SDL2/SDL.h>

namespace swirski::inputs::keyboard
{

    void initialise(SDL_Event &event, bool &running)
    {

        while (SDL_PollEvent(&event))
        {

            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            if (event.type == SDL_KEYDOWN)
            {

                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    running = false;
                }

                // if left arrow pressed
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    std::cout << "Left arrow key pressed" << std::endl;

                    swirski::input::handleInput(
                        swirski::input::input_action::Previous);
                }

                // if right arrow pressed
                if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    std::cout << "Right arrow key pressed" << std::endl;

                    swirski::input::handleInput(
                        swirski::input::input_action::Next);
                }

                // if enter key pressed
                if (event.key.keysym.sym == SDLK_RETURN)
                {
                    std::cout << "Enter key pressed" << std::endl;

                    swirski::input::handleInput(
                        swirski::input::input_action::Confirm);
                }

                // if backspace key pressed
                if (event.key.keysym.sym == SDLK_BACKSPACE)
                {
                    std::cout << "Backspace key pressed" << std::endl;

                    swirski::input::handleInput(
                        swirski::input::input_action::Back);
                }

                // if H key pressed
                if (event.key.keysym.sym == SDLK_h)
                {
                    std::cout << "H key pressed" << std::endl;

                    swirski::input::handleInput(
                        swirski::input::input_action::Home);
                }
            }
        }
    }

}
