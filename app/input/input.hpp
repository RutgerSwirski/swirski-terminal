#pragma once

namespace swirski::input
{
    enum class InputAction
    {
        Previous,
        Next,
        Confirm,
        Back,
        Home
    };

    void handleInput(InputAction action);
}