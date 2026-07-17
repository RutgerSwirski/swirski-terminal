#pragma once

#include "transport.hpp"

#include <cstddef>
#include <cstdint>
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
        void sendNextQueuedFrame();

        NimBLEServer *server = nullptr;

        NimBLECharacteristic *receiveCharacteristic =
            nullptr;

        NimBLECharacteristic *transmitCharacteristic =
            nullptr;

        struct IncomingMessage
        {
            std::string message;
            std::uint16_t connHandle = 0;
        };

        std::queue<IncomingMessage> incomingMessages;
        std::mutex incomingMessagesMutex;

        struct OutgoingFrame
        {
            std::string frame;
            std::uint16_t connHandle = 0;
            std::size_t frameIndex = 0;
            std::size_t frameCount = 0;
        };

        std::queue<OutgoingFrame> outgoingFrames;
        std::mutex outgoingFramesMutex;

        enum class ConnectionEvent
        {
            Connected,
            Disconnected
        };

        std::optional<ConnectionEvent>
            pendingConnectionEvent;

        std::optional<std::uint16_t>
            pendingDisconnectedConnHandle;

        std::mutex connectionEventMutex;
    };

}
