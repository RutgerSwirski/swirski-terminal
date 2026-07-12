#pragma once

namespace swirski::input
{
    enum class input_action
    {
        Previous,
        Next,
        Confirm,
        Back,
        Home
    };

    void handleInput(input_action action);
}