#pragma once

#include <optional>
#include <string>

namespace swirski::protocol
{
    std::optional<std::string> handleIncomingMessage(
        const std::string &rawMessage);
}