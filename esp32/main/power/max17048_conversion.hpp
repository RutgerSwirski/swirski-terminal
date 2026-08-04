#pragma once

#include <algorithm>
#include <cstdint>

namespace swirski::hardware::max17048
{
    constexpr std::uint16_t voltageMillivoltsFromRaw(
        std::uint16_t rawVoltage)
    {
        // VCELL has a 78.125 uV LSB, or 5 / 64 mV.
        return static_cast<std::uint16_t>(
            (static_cast<std::uint32_t>(rawVoltage) * 5U + 32U) /
            64U);
    }

    constexpr std::uint8_t percentageFromRaw(
        std::uint16_t rawPercentage)
    {
        // SOC has 1 / 256 percent resolution. Round for the compact UI.
        const std::uint16_t roundedPercentage =
            static_cast<std::uint16_t>(
                (static_cast<std::uint32_t>(rawPercentage) + 128U) /
                256U);

        return static_cast<std::uint8_t>(
            std::min<std::uint16_t>(roundedPercentage, 100U));
    }
}
