#include "settings_service.hpp"

namespace swirski::service::settings
{
    namespace
    {
        PowerMode powerMode = PowerMode::Balanced;
        PowerModeHandler powerModeHandler = nullptr;
    }

    void setPowerModeHandler(PowerModeHandler handler)
    {
        powerModeHandler = handler;

        if (powerModeHandler != nullptr)
        {
            powerModeHandler(powerMode);
        }
    }

    PowerMode getPowerMode()
    {
        return powerMode;
    }

    void setPowerMode(PowerMode newMode)
    {
        powerMode = newMode;

        if (powerModeHandler != nullptr)
        {
            powerModeHandler(powerMode);
        }
    }
}
