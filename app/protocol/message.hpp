#pragma once

#include <string>

#include "message_type.hpp"

namespace swirski::protocol
{
    struct Message
    {
        int version = 1;

        MessageType type = MessageType::Unknown;

        std::string id;
    };
}