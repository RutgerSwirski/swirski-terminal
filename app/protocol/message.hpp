#pragma once

#include <string>

#include "message_type.hpp"

namespace swirski::protocol
{

    struct Payload
    {
        std::string notificationId;
        std::string appName;
        std::string title;
        std::string body;
    };

    struct Message
    {
        int version = 1;
        MessageType type = MessageType::Unknown;
        std::string id;
        Payload payload;
    };
}