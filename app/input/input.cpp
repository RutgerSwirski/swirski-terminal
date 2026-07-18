#include "input.hpp"

#include "screen_manager.hpp"

namespace swirski::input
{
    void handleInput(input_action action)
    {
        swirski::screens::manager::handleInput(action);
    }
}
