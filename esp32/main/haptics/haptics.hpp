#pragma once

namespace swirski::hardware::haptics
{
    enum class Effect
    {
        Tick,
        Notification
    };

    void initialise();
    void play(Effect effect);
    void update();
}