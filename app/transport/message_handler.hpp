// this files defines the message handler for the transport layer - eg bluetooth or websocket
#pragma once

#include <string>

namespace swirski::transport::message_handler
{
    void handleMessage(std::string message);
}