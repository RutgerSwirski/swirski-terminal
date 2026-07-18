#pragma once

#include <string>

#include "input.hpp"

namespace swirski::ui::keyboard
{
    using SubmitHandler = void (*)(const std::string &text);

    void open(
        const std::string &initialText,
        SubmitHandler onSubmit,
        bool obscureText = false);
    void render();
    void handleInput(swirski::input::input_action action);
}
