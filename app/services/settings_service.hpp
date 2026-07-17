#pragma once

namespace swirski::service::settings
{
    enum class PowerMode
    {
        Performance,
        Balanced,
        Saver
    };

    using PowerModeHandler = void (*)(PowerMode mode);

    void setPowerModeHandler(PowerModeHandler handler);

    PowerMode getPowerMode();
    void setPowerMode(PowerMode mode);
}
