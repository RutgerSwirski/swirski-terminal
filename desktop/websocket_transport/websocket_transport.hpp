
#pragma once

#include "transport.hpp"

#include <string>

namespace swirski::transport::websocket
{
    class WebSocketTransport : public swirski::transport::Transport
    {
    public:
        void initialise() override;
        void update() override;
        void send(const std::string &message) override;
    };

}