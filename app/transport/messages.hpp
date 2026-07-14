// this file defines the messages that can be sent over the transport layer
#pragma once

namespace swirski::transport::messages
{
    enum class MessageType
    {
        Notification,
        SystemData
    };
}