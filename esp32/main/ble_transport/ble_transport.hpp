#pragma once

#include "transport.hpp"

#include <mutex>
#include <queue>
#include <string>
#include <optional>

class NimBLEServer;
class NimBLECharacteristic;

namespace swirski::transport::ble
{
    class BleTransport : public swirski::transport::Transport
    {
    public:
        void initialise() override;
        void update() override;

        void send(const std::string &message) override;

    private:
        NimBLEServer *server = nullptr;

        NimBLECharacteristic *receiveCharacteristic =
            nullptr;

        NimBLECharacteristic *transmitCharacteristic =
            nullptr;

        std::queue<std::string> incomingMessages;
        std::mutex incomingMessagesMutex;

        enum class ConnectionEvent
        {
            Connected,
            Disconnected
        };

        std::optional<ConnectionEvent>
            pendingConnectionEvent;

        std::mutex connectionEventMutex;
    };

}