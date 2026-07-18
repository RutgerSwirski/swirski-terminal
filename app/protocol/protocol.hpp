#pragma once

#include <optional>
#include <string>
#include "message.hpp"

namespace swirski::protocol
{
    struct MessageResult
    {
        std::optional<std::string> response;
        bool disconnectRequested = false;
    };

    MessageResult handleIncomingMessage(
        const std::string &rawMessage);

    std::optional<Message> parseMessage(const std::string &rawMessage);

    std::string createPongMessage(const std::string &messageId);
}
