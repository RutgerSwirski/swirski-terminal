#include "websocket_transport.hpp"

#include <iostream>

#include <ixwebsocket/IXWebSocketServer.h>

#include <mutex>
#include <queue>
#include <string>

namespace
{
    constexpr int PORT = 8008;
    constexpr char HOST[] = "0.0.0.0";

    ix::WebSocketServer server(PORT, HOST);
}

namespace swirski::transport::websocket
{
    void WebSocketTransport::initialise()
    {
        server.setOnClientMessageCallback(
            [this](
                std::shared_ptr<ix::ConnectionState> connectionState,
                ix::WebSocket &webSocket,
                const ix::WebSocketMessagePtr &message)
            {
                if (
                    message->type ==
                    ix::WebSocketMessageType::Open)
                {
                    std::cout
                        << "New connection from: "
                        << connectionState->getRemoteIp()
                        << std::endl;

                    std::cout
                        << "Connection ID: "
                        << connectionState->getId()
                        << std::endl;

                    // send a welcome message
                    webSocket.send(
                        "Welcome to the WebSocket server");
                }

                else if (
                    message->type ==
                    ix::WebSocketMessageType::Message)
                {
                    std::cout
                        << "Received: "
                        << message->str
                        << std::endl;

                    {
                        std::lock_guard<std::mutex> lock(
                            incomingMessagesMutex);

                        incomingMessages.push(
                            message->str);
                    }

                    webSocket.send(
                        message->str,
                        message->binary);
                }
                else if (
                    message->type ==
                    ix::WebSocketMessageType::Close)
                {
                    std::cout
                        << "Client disconnected"
                        << std::endl;
                }
                else if (
                    message->type ==
                    ix::WebSocketMessageType::Error)
                {
                    std::cerr
                        << "WebSocket error: "
                        << message->errorInfo.reason
                        << std::endl;
                }
            });

        server.disablePerMessageDeflate();

        const auto result = server.listen();

        if (!result.first)
        {
            std::cerr
                << "Could not start WebSocket server: "
                << result.second
                << std::endl;

            return;
        }

        server.start();

        std::cout
            << "WebSocket server listening on port "
            << PORT
            << std::endl;
    }

    void WebSocketTransport::update()
    {
        std::string nextMessage;

        {
            std::lock_guard<std::mutex> lock(
                incomingMessagesMutex);

            if (incomingMessages.empty())
            {
                return;
            }

            nextMessage =
                std::move(incomingMessages.front());

            incomingMessages.pop();
        }

        std::cout
            << "Processing message in app thread: "
            << nextMessage
            << std::endl;
    }

    void WebSocketTransport::send(
        const std::string &message)
    {
        std::cout
            << "Application wants to send: "
            << message
            << std::endl;

        // We still need to retain/find a connected client
        // before this can actually transmit.
    }
}