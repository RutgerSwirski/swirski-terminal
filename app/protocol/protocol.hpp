#pragma once

#include <optional>
#include <string>
#include "message.hpp"

namespace swirski::protocol
{
    std::optional<std::string> handleIncomingMessage(
        const std::string &rawMessage);

    std::optional<Message> parseMessage(const std::string &rawMessage);

    std::string createPongMessage(const std::string &messageId);
}