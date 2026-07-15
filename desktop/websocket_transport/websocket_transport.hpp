
#pragma once

#include "transport.hpp"

#include <mutex>
#include <queue>
#include <string>

namespace swirski::transport::websocket
{
    class WebSocketTransport : public swirski::transport::Transport
    {
    public:
        void initialise() override;
        void update() override;
        void send(const std::string &message) override;

    private:
        std::queue<std::string> incomingMessages;
        std::mutex incomingMessagesMutex;

        bool connectionStateRefreshPending = false;
        std::mutex connectionStateMutex;
    };

}