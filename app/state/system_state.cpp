
#include "system_state.hpp"

#include <iostream>

namespace swirski::state::system
{

    namespace
    {
        SystemStateSnapshot currentState;

    }

    SystemStateSnapshot getSnapshot()
    {
        return currentState;
    }

    void setConnection(
        TransportType transport,
        ConnectionStatus status)
    {

        if (
            currentState.connection.transport == transport &&
            currentState.connection.status == status)
        {
            return;
        }

        currentState.connection.transport = transport;
        currentState.connection.status = status;
        currentState.revision++;

        std::cout
            << "System-state revision: "
            << currentState.revision
            << std::endl;
    }

    void setBatteryPercent(
        std::optional<std::uint8_t> percentage)
    {

        if (
            percentage.has_value() &&
            *percentage > 100)
        {
            return;
        }

        if (
            currentState.batteryPercent == percentage)
        {
            return;
        }

        currentState.batteryPercent = percentage;
        currentState.revision++;
    }

    void setCharging(
        bool isCharging)
    {
        if (
            currentState.charging == isCharging)
        {
            return;
        }

        currentState.charging = isCharging;
        currentState.revision++;
    }

}