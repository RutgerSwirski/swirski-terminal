

#include "websocket_transport.hpp"

#include <iostream>

namespace swirski::transport::websocket
{

    void WebSocketTransport::initialise()
    {

        std::cout << "Initialising websocket transport" << std::endl;
    }

    void WebSocketTransport::update()
    {
        std::cout << "Updating websocket transport" << std::endl;
    }

    void WebSocketTransport::send(const std::string &message)
    {
        std::cout << "Sending websocket message: " << message << std::endl;
    }
}