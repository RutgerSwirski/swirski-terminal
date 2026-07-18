#pragma once

namespace swirski::transport::ble_security
{
    constexpr bool isSecure(
        bool encrypted,
        bool authenticated,
        bool bonded)
    {
        return encrypted && authenticated && bonded;
    }
}
