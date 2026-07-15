#include "websocket_transport.hpp"

#include <iostream>

#include <ixwebsocket/IXWebSocketServer.h>

#include <mutex>
#include <queue>
#include <string>

#include "protocol.hpp"

#include "system_state.hpp"

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
        swirski::state::system::setConnection(
            swirski::state::system::TransportType::WebSocket,
            swirski::state::system::ConnectionStatus::Connecting);

        server.setOnClientMessageCallback(
            [this](
                std::shared_ptr<ix::ConnectionState> connectionState,
                ix::WebSocket &webSocket,
                const ix::WebSocketMessagePtr &message)
            {
                const auto requestConnectionStateRefresh =
                    [this]()
                {
                    std::lock_guard<std::mutex> lock(
                        connectionStateMutex);

                    connectionStateRefreshPending = true;
                };

                if (
                    message->type ==
                    ix::WebSocketMessageType::Open)
                {
                    requestConnectionStateRefresh();

                    std::cout
                        << "New connection from: "
                        << connectionState->getRemoteIp()
                        << std::endl;

                    std::cout
                        << "Connection ID: "
                        << connectionState->getId()
                        << std::endl;
                }

                else if (
                    message->type ==
                    ix::WebSocketMessageType::Message)
                {
                    std::cout
                        << "Received WebSocket message: "
                        << message->str
                        << std::endl;

                    {
                        std::lock_guard<std::mutex> lock(
                            incomingMessagesMutex);

                        incomingMessages.push(
                            message->str);
                    }
                }
                else if (
                    message->type ==
                    ix::WebSocketMessageType::Close)
                {
                    requestConnectionStateRefresh();

                    std::cout
                        << "Client disconnected"
                        << std::endl;

                    std::cout
                        << "Connection ID: "
                        << connectionState->getId()
                        << std::endl;

                    // Close the connection
                    // webSocket.close();
                }
                else if (
                    message->type ==
                    ix::WebSocketMessageType::Error)
                {
                    requestConnectionStateRefresh();

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
            swirski::state::system::setConnection(
                swirski::state::system::TransportType::WebSocket,
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not start WebSocket server: "
                << result.second
                << std::endl;

            return;
        }

        server.start();

        swirski::state::system::setConnection(
            swirski::state::system::TransportType::WebSocket,
            swirski::state::system::ConnectionStatus::Disconnected);

        std::cout
            << "WebSocket server listening on port "
            << PORT
            << std::endl;
    }

    void WebSocketTransport::update()
    {
        bool shouldRefreshConnectionState = false;

        {
            std::lock_guard<std::mutex> lock(
                connectionStateMutex);

            shouldRefreshConnectionState =
                connectionStateRefreshPending;

            connectionStateRefreshPending = false;
        }

        if (shouldRefreshConnectionState)
        {
            const auto clients =
                server.getClients();

            const auto status =
                clients.empty()
                    ? swirski::state::system::ConnectionStatus::Disconnected
                    : swirski::state::system::ConnectionStatus::Connected;

            swirski::state::system::setConnection(
                swirski::state::system::TransportType::WebSocket,
                status);

            std::cout
                << "WebSocket clients connected: "
                << clients.size()
                << std::endl;
        }

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
            << "Processing WebSocket message in app thread: "
            << nextMessage
            << std::endl;

        const auto response =
            swirski::protocol::handleIncomingMessage(
                nextMessage);

        if (response)
        {
            send(*response);
        }
    }

    void WebSocketTransport::send(
        const std::string &message)
    {
        const auto clients =
            server.getClients();

        if (clients.empty())
        {
            std::cout
                << "No WebSocket client connected"
                << std::endl;

            return;
        }

        for (const auto &client : clients)
        {
            client->send(message);
        }

        std::cout
            << "Sent WebSocket message: "
            << message
            << std::endl;
    }
}