

#include "ble_transport.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <tuple>
#include <vector>

#include <NimBLEDevice.h>
#include "esp_random.h"

#include "ble_security.hpp"
#include "protocol.hpp"
#include "system_state.hpp"
#include "notification_toast.hpp"

namespace
{
    constexpr char SERVICE_UUID[] =
        "ee62dc78-0a3d-4c78-987d-23b73d5ae9d9";

    constexpr char RECEIVE_CHARACTERISTIC_UUID[] =
        "0c83378b-52ed-4500-bc9e-5bb153bbd4d5";

    constexpr char TRANSMIT_CHARACTERISTIC_UUID[] =
        "0c83378b-52ed-4500-bc9e-5bb153bbd4d6";

    constexpr std::uint8_t FRAME_MAGIC = 0x53;
    constexpr std::uint8_t FRAME_VERSION = 1;

    constexpr std::size_t ATT_HEADER_BYTES = 3;
    constexpr std::size_t FRAME_HEADER_BYTES = 8;

    constexpr std::size_t MAX_MESSAGE_BYTES = 64 * 1024;
    constexpr std::size_t MAX_CHUNK_COUNT = 255;
    constexpr std::uint8_t SYNC_TOAST_MIN_CHUNKS = 5;

    constexpr std::chrono::milliseconds TRANSFER_TIMEOUT{
        3000};
    constexpr std::chrono::milliseconds SEND_RETRY_DELAY{
        1000};

    using Clock = std::chrono::steady_clock;

    std::uint32_t nextOutgoingMessageId = 1;
    Clock::time_point nextSendAttemptAt =
        Clock::time_point::min();

    bool shouldLogFrameProgress(
        std::size_t frameIndex,
        std::size_t frameCount)
    {
        return frameIndex == 1 ||
               frameIndex == frameCount ||
               frameIndex % 10 == 0;
    }

    std::uint32_t readUint32LittleEndian(
        const std::string &bytes,
        std::size_t offset)
    {
        return static_cast<std::uint32_t>(
                   static_cast<std::uint8_t>(
                       bytes[offset])) |
               (static_cast<std::uint32_t>(
                    static_cast<std::uint8_t>(
                        bytes[offset + 1]))
                << 8) |
               (static_cast<std::uint32_t>(
                    static_cast<std::uint8_t>(
                        bytes[offset + 2]))
                << 16) |
               (static_cast<std::uint32_t>(
                    static_cast<std::uint8_t>(
                        bytes[offset + 3]))
                << 24);
    }

    void writeUint32LittleEndian(
        std::string &bytes,
        std::size_t offset,
        std::uint32_t value)
    {
        bytes[offset] =
            static_cast<char>(value & 0xff);

        bytes[offset + 1] =
            static_cast<char>((value >> 8) & 0xff);

        bytes[offset + 2] =
            static_cast<char>((value >> 16) & 0xff);

        bytes[offset + 3] =
            static_cast<char>((value >> 24) & 0xff);
    }

    struct PendingMessageKey
    {
        std::uint16_t connHandle;
        std::uint32_t messageId;

        bool operator<(
            const PendingMessageKey &other) const
        {
            return std::tie(
                       connHandle,
                       messageId) <
                   std::tie(
                       other.connHandle,
                       other.messageId);
        }
    };

    struct PendingMessage
    {
        explicit PendingMessage(
            std::uint8_t expectedChunkCount,
            Clock::time_point now)
            : chunkCount(expectedChunkCount),
              chunks(expectedChunkCount),
              lastUpdatedAt(now)
        {
        }

        std::uint8_t chunkCount;
        std::vector<std::optional<std::string>> chunks;

        std::size_t receivedChunkCount = 0;
        std::size_t receivedByteCount = 0;

        Clock::time_point lastUpdatedAt;
    };

    class FrameAssembler
    {
    public:
        std::optional<std::string> acceptFrame(
            const std::string &frame,
            std::uint16_t connHandle)
        {
            const Clock::time_point now =
                Clock::now();

            clearExpiredMessages(now);

            if (frame.size() < FRAME_HEADER_BYTES)
            {
                std::cerr
                    << "Rejected BLE frame: header is incomplete"
                    << std::endl;

                return std::nullopt;
            }

            const auto magic =
                static_cast<std::uint8_t>(
                    frame[0]);

            const auto version =
                static_cast<std::uint8_t>(
                    frame[1]);

            const std::uint32_t messageId =
                readUint32LittleEndian(
                    frame,
                    2);

            const auto chunkIndex =
                static_cast<std::uint8_t>(
                    frame[6]);

            const auto chunkCount =
                static_cast<std::uint8_t>(
                    frame[7]);

            if (magic != FRAME_MAGIC)
            {
                std::cerr
                    << "Rejected BLE frame: invalid magic byte"
                    << std::endl;

                return std::nullopt;
            }

            if (version != FRAME_VERSION)
            {
                std::cerr
                    << "Rejected BLE frame: unsupported version "
                    << static_cast<int>(version)
                    << std::endl;

                return std::nullopt;
            }

            if (
                chunkCount == 0 ||
                chunkIndex >= chunkCount)
            {
                std::cerr
                    << "Rejected BLE frame: invalid chunk information"
                    << std::endl;

                return std::nullopt;
            }

            const PendingMessageKey key{
                connHandle,
                messageId};

            auto [iterator, inserted] =
                pendingMessages.try_emplace(
                    key,
                    chunkCount,
                    now);

            PendingMessage &pending =
                iterator->second;

            pending.lastUpdatedAt =
                now;

            if (
                !inserted &&
                pending.chunkCount != chunkCount)
            {
                std::cerr
                    << "Rejected BLE frame: chunk count changed"
                    << std::endl;

                pendingMessages.erase(iterator);

                return std::nullopt;
            }

            const std::string payload =
                frame.substr(FRAME_HEADER_BYTES);

            if (
                !pending.chunks[chunkIndex]
                     .has_value())
            {
                if (
                    pending.receivedByteCount +
                        payload.size() >
                    MAX_MESSAGE_BYTES)
                {
                    std::cerr
                        << "Rejected BLE message: message is too large"
                        << std::endl;

                    pendingMessages.erase(key);

                    return std::nullopt;
                }

                pending.chunks[chunkIndex] =
                    payload;

                pending.receivedChunkCount += 1;
                pending.receivedByteCount +=
                    payload.size();
            }

            const std::size_t frameIndex =
                static_cast<std::size_t>(
                    chunkIndex) +
                1;

            if (
                shouldLogFrameProgress(
                    frameIndex,
                    chunkCount))
            {
                std::cout
                    << "Received BLE frame "
                    << frameIndex
                    << "/"
                    << static_cast<int>(chunkCount)
                    << " for message "
                    << messageId
                    << std::endl;
            }

            if (chunkCount >= SYNC_TOAST_MIN_CHUNKS)
            {
                swirski::ui::notification_toast::
                    requestSyncProgress(
                        static_cast<int>(
                            (
                                pending.receivedChunkCount *
                                100) /
                            pending.chunkCount));
            }

            if (
                pending.receivedChunkCount !=
                pending.chunkCount)
            {
                return std::nullopt;
            }

            std::string completeMessage;

            completeMessage.reserve(
                pending.receivedByteCount);

            for (const auto &chunk : pending.chunks)
            {
                if (!chunk)
                {
                    return std::nullopt;
                }

                completeMessage += *chunk;
            }

            pendingMessages.erase(key);

            std::cout
                << "Reassembled BLE message "
                << messageId
                << ": "
                << completeMessage.size()
                << " bytes"
                << std::endl;

            swirski::ui::notification_toast::
                clearSyncProgress();

            return completeMessage;
        }

        void clearConnection(
            std::uint16_t connHandle)
        {
            for (
                auto iterator =
                    pendingMessages.begin();
                iterator != pendingMessages.end();)
            {
                if (
                    iterator->first.connHandle ==
                    connHandle)
                {
                    iterator =
                        pendingMessages.erase(iterator);
                }
                else
                {
                    ++iterator;
                }
            }
        }

        void clearExpiredMessages()
        {
            clearExpiredMessages(
                Clock::now());
        }

    private:
        void clearExpiredMessages(
            Clock::time_point now)
        {
            for (
                auto iterator =
                    pendingMessages.begin();
                iterator != pendingMessages.end();)
            {
                if (
                    now - iterator->second.lastUpdatedAt >
                    TRANSFER_TIMEOUT)
                {
                    iterator =
                        pendingMessages.erase(iterator);
                }
                else
                {
                    ++iterator;
                }
            }
        }

        std::map<
            PendingMessageKey,
            PendingMessage>
            pendingMessages;
    };

    FrameAssembler incomingFrameAssembler;

    std::vector<std::string>
    encodeMessageIntoFrames(
        const std::string &message,
        std::uint16_t mtu)
    {
        if (message.size() > MAX_MESSAGE_BYTES)
        {
            std::cerr
                << "Cannot frame BLE message: message is too large"
                << std::endl;

            return {};
        }

        if (
            mtu <=
            ATT_HEADER_BYTES +
                FRAME_HEADER_BYTES)
        {
            std::cerr
                << "Cannot frame BLE message: MTU is too small"
                << std::endl;

            return {};
        }

        const std::size_t maximumFrameBytes =
            mtu - ATT_HEADER_BYTES;

        const std::size_t maximumPayloadBytes =
            maximumFrameBytes -
            FRAME_HEADER_BYTES;

        const std::size_t chunkCountValue =
            std::max<std::size_t>(
                1,
                (
                    message.size() +
                    maximumPayloadBytes -
                    1) /
                    maximumPayloadBytes);

        if (chunkCountValue > MAX_CHUNK_COUNT)
        {
            std::cerr
                << "Cannot frame BLE message: too many chunks"
                << std::endl;

            return {};
        }

        const auto chunkCount =
            static_cast<std::uint8_t>(
                chunkCountValue);

        const std::uint32_t messageId =
            nextOutgoingMessageId;

        nextOutgoingMessageId += 1;

        if (nextOutgoingMessageId == 0)
        {
            nextOutgoingMessageId = 1;
        }

        std::vector<std::string> frames;

        frames.reserve(chunkCount);

        for (
            std::size_t chunkIndex = 0;
            chunkIndex < chunkCount;
            ++chunkIndex)
        {
            const std::size_t payloadStart =
                chunkIndex *
                maximumPayloadBytes;

            const std::size_t payloadSize =
                std::min(
                    maximumPayloadBytes,
                    message.size() -
                        payloadStart);

            std::string frame(
                FRAME_HEADER_BYTES +
                    payloadSize,
                '\0');

            frame[0] =
                static_cast<char>(
                    FRAME_MAGIC);

            frame[1] =
                static_cast<char>(
                    FRAME_VERSION);

            writeUint32LittleEndian(
                frame,
                2,
                messageId);

            frame[6] =
                static_cast<char>(
                    chunkIndex);

            frame[7] =
                static_cast<char>(
                    chunkCount);

            std::copy(
                message.begin() +
                    payloadStart,
                message.begin() +
                    payloadStart +
                    payloadSize,
                frame.begin() +
                    FRAME_HEADER_BYTES);

            frames.push_back(
                std::move(frame));
        }

        return frames;
    }

    class ServerCallbacks : public NimBLEServerCallbacks
    {
    public:
        void setConnectionHandler(
            std::function<void(bool, std::uint16_t)> handler)
        {
            connectionHandler =
                std::move(handler);
        }

        void onConnect(
            NimBLEServer *,
            NimBLEConnInfo &connInfo) override
        {
            if (connectionHandler)
            {
                connectionHandler(
                    true,
                    connInfo.getConnHandle());
            }
        }

        void onDisconnect(
            NimBLEServer *,
            NimBLEConnInfo &connInfo,
            int) override
        {
            if (connectionHandler)
            {
                connectionHandler(
                    false,
                    connInfo.getConnHandle());
            }
        }

        std::uint32_t onPassKeyDisplay() override
        {
            const std::uint32_t passkey =
                100000 + (esp_random() % 900000);

            const std::string body =
                "Code: " + std::to_string(passkey);

            swirski::ui::notification_toast::requestMessage(
                "Bluetooth pairing",
                body.c_str(),
                30000);

            return passkey;
        }

        void onAuthenticationComplete(
            NimBLEConnInfo &connInfo) override
        {
            const bool secure =
                swirski::transport::ble_security::isSecure(
                    connInfo.isEncrypted(),
                    connInfo.isAuthenticated(),
                    connInfo.isBonded());

            swirski::ui::notification_toast::requestMessage(
                "Bluetooth",
                secure ? "Pairing complete" : "Pairing failed");

            if (!secure)
            {
                NimBLEDevice::getServer()->disconnect(
                    connInfo.getConnHandle());
            }
        }

    private:
        std::function<void(bool, std::uint16_t)> connectionHandler;
    };

    class ReceiveCallbacks : public NimBLECharacteristicCallbacks
    {
    public:
        void setMessageHandler(
            std::function<void(const std::string &, std::uint16_t)> handler)
        {
            messageHandler = std::move(handler);
        }
        void onWrite(
            NimBLECharacteristic *characteristic,
            NimBLEConnInfo &connInfo) override
        {
            if (!swirski::transport::ble_security::isSecure(
                    connInfo.isEncrypted(),
                    connInfo.isAuthenticated(),
                    connInfo.isBonded()))
            {
                std::cerr
                    << "Rejected unsecure BLE write"
                    << std::endl;
                return;
            }

            const std::string frameBytes =
                characteristic->getValue();

            const std::uint16_t connHandle =
                connInfo.getConnHandle();

            if (messageHandler)
            {
                messageHandler(
                    frameBytes,
                    connHandle);
            }
        }

    private:
        std::function<void(const std::string &, std::uint16_t)> messageHandler;
    };

    ReceiveCallbacks receiveCallbacks;
    ServerCallbacks serverCallbacks;

}

namespace swirski::transport::ble
{
    void BleTransport::initialise()
    {
        const auto setBleStatus =
            [](swirski::state::system::ConnectionStatus status)
        {
            swirski::state::system::setConnection(
                swirski::state::system::TransportType::Ble,
                status);
        };

        std::cout << "Initialising BLE Server" << std::endl;

        setBleStatus(
            swirski::state::system::ConnectionStatus::Connecting);

        const bool initialised = NimBLEDevice::init("Swirski Terminal");

        if (!initialised)
        {
            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not initialise NimBLE"
                << std::endl;

            return;
        }

        const bool mtuConfigured = NimBLEDevice::setMTU(247);

        if (!mtuConfigured)
        {
            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not configure MTU"
                << std::endl;
            return;
        }

        NimBLEDevice::setSecurityIOCap(
            BLE_HS_IO_DISPLAY_ONLY);
        NimBLEDevice::setSecurityAuth(
            true,
            true,
            true);

        server = NimBLEDevice::createServer();

        if (server == nullptr)
        {
            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not create BLE server"
                << std::endl;
            return;
        }

        serverCallbacks.setConnectionHandler(
            [this](
                bool connected,
                std::uint16_t connHandle)
            {
                std::lock_guard<std::mutex> lock(
                    connectionEventMutex);

                pendingConnectionEvent =
                    connected
                        ? ConnectionEvent::Connected
                        : ConnectionEvent::Disconnected;

                if (!connected)
                {
                    pendingDisconnectedConnHandle =
                        connHandle;
                }
            });

        server->setCallbacks(
            &serverCallbacks,
            false);

        server->advertiseOnDisconnect(true);

        NimBLEService *service = server->createService(SERVICE_UUID);

        if (service == nullptr)
        {
            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not create BLE service"
                << std::endl;
            return;
        }

        receiveCharacteristic = service->createCharacteristic(
            RECEIVE_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::WRITE |
                NIMBLE_PROPERTY::WRITE_ENC);

        transmitCharacteristic = service->createCharacteristic(
            TRANSMIT_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ |
                NIMBLE_PROPERTY::NOTIFY |
                NIMBLE_PROPERTY::READ_ENC);

        if (receiveCharacteristic == nullptr || transmitCharacteristic == nullptr)
        {
            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not create BLE characteristic"
                << std::endl;
            return;
        }

        receiveCallbacks.setMessageHandler(
            [this](
                const std::string &message,
                std::uint16_t connHandle)
            {
                std::lock_guard<std::mutex> lock(
                    this->incomingMessagesMutex);

                this->incomingMessages.push(
                    IncomingMessage{
                        message,
                        connHandle});
            });

        receiveCharacteristic->setCallbacks(&receiveCallbacks);

        transmitCharacteristic->setValue("Hello World");

        const bool serverStarted = server->start();

        if (!serverStarted)
        {

            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not start BLE server"
                << std::endl;

            return;
        }

        NimBLEAdvertising *advertising = server->getAdvertising();

        if (advertising == nullptr)
        {

            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not get BLE advertising"
                << std::endl;
            return;
        }

        advertising->enableScanResponse(true);

        const bool nameSet = advertising->setName("Swirski Terminal");

        if (!nameSet)
        {
            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not set name to BLE advertising"
                << std::endl;
            return;
        }

        const bool serviceUuidAdded =
            advertising->addServiceUUID(SERVICE_UUID);

        if (!serviceUuidAdded)
        {

            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not add service UUID to BLE advertising"
                << std::endl;
            return;
        }

        const bool advertisingStarted = advertising->start();

        if (!advertisingStarted)
        {

            setBleStatus(
                swirski::state::system::ConnectionStatus::Error);

            std::cerr
                << "Could not start BLE advertising"
                << std::endl;

            return;
        }

        setBleStatus(
            swirski::state::system::ConnectionStatus::Disconnected);

        std::cout << "BLE Server started and advertising" << std::endl;
    }

    void BleTransport::update()
    {
        incomingFrameAssembler.clearExpiredMessages();

        std::optional<ConnectionEvent> connectionEvent;
        std::optional<std::uint16_t> disconnectedConnHandle;

        {
            std::lock_guard<std::mutex> lock(
                connectionEventMutex);

            connectionEvent = pendingConnectionEvent;
            disconnectedConnHandle =
                pendingDisconnectedConnHandle;

            pendingConnectionEvent.reset();
            pendingDisconnectedConnHandle.reset();
        }

        if (disconnectedConnHandle)
        {
            swirski::ui::notification_toast::
                clearSyncProgress();

            incomingFrameAssembler.clearConnection(
                *disconnectedConnHandle);

            std::lock_guard<std::mutex> lock(
                outgoingFramesMutex);

            std::queue<OutgoingFrame> remainingFrames;

            while (!outgoingFrames.empty())
            {
                OutgoingFrame frame =
                    std::move(outgoingFrames.front());

                outgoingFrames.pop();

                if (
                    frame.connHandle !=
                    *disconnectedConnHandle)
                {
                    remainingFrames.push(
                        std::move(frame));
                }
            }

            outgoingFrames =
                std::move(remainingFrames);
        }

        if (connectionEvent)
        {
            switch (*connectionEvent)
            {
            case ConnectionEvent::Connected:
                swirski::state::system::setConnection(
                    swirski::state::system::TransportType::Ble,
                    swirski::state::system::ConnectionStatus::Connected);

                std::cout << "BLE connected" << std::endl;
                break;
            case ConnectionEvent::Disconnected:
                swirski::state::system::setConnection(
                    swirski::state::system::TransportType::Ble,
                    swirski::state::system::ConnectionStatus::Disconnected);

                std::cout << "BLE disconnected" << std::endl;
                break;
            }
        }

        sendNextQueuedFrame();

        IncomingMessage nextMessage;

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

        const auto completeMessage =
            incomingFrameAssembler.acceptFrame(
                nextMessage.message,
                nextMessage.connHandle);

        if (!completeMessage)
        {
            // The message needs more frames.
            return;
        }

        std::cout
            << "Processing complete BLE message: "
            << completeMessage->size()
            << " bytes"
            << std::endl;

        const auto result =
            swirski::protocol::handleIncomingMessage(
                *completeMessage);

        if (
            result.disconnectRequested)
        {
            if (server == nullptr)
            {
                std::cerr
                    << "Could not disconnect BLE client because server is null"
                    << std::endl;

                return;
            }

            const bool disconnected =
                server->disconnect(
                    nextMessage.connHandle);

            if (!disconnected)
            {
                std::cerr
                    << "Could not disconnect BLE client"
                    << std::endl;
            }

            return;
        }

        if (result.response)
        {
            send(*result.response);
        }
    }

    void BleTransport::sendNextQueuedFrame()
    {
        const auto now = Clock::now();

        if (now < nextSendAttemptAt)
        {
            return;
        }

        if (transmitCharacteristic == nullptr)
        {
            return;
        }

        OutgoingFrame nextFrame;

        {
            std::lock_guard<std::mutex> lock(
                outgoingFramesMutex);

            if (outgoingFrames.empty())
            {
                return;
            }

            nextFrame =
                outgoingFrames.front();
        }

        const bool sent =
            transmitCharacteristic->notify(
                reinterpret_cast<
                    const std::uint8_t *>(
                    nextFrame.frame.data()),
                nextFrame.frame.size(),
                nextFrame.connHandle);

        if (!sent)
        {
            nextSendAttemptAt =
                now + SEND_RETRY_DELAY;

            std::cerr
                << "Could not send queued BLE frame "
                << nextFrame.frameIndex
                << "/"
                << nextFrame.frameCount
                << std::endl;

            return;
        }

        nextSendAttemptAt =
            Clock::time_point::min();

        {
            std::lock_guard<std::mutex> lock(
                outgoingFramesMutex);

            if (!outgoingFrames.empty())
            {
                outgoingFrames.pop();
            }
        }

        if (
            shouldLogFrameProgress(
                nextFrame.frameIndex,
                nextFrame.frameCount))
        {
            std::cout
                << "Sent queued BLE frame "
                << nextFrame.frameIndex
                << "/"
                << nextFrame.frameCount
                << std::endl;
        }
    }

    void BleTransport::send(
        const std::string &message)
    {
        if (transmitCharacteristic == nullptr)
        {
            std::cerr
                << "Transmit characteristic is not initialised"
                << std::endl;

            return;
        }

        if (server == nullptr)
        {
            std::cerr
                << "BLE server is not initialised"
                << std::endl;

            return;
        }

        const auto peerHandles =
            server->getPeerDevices();

        if (peerHandles.empty())
        {
            std::cout
                << "No BLE client connected"
                << std::endl;

            return;
        }

        // Your MVP currently supports one phone.
        const std::uint16_t connHandle =
            peerHandles.front();

        const std::uint16_t peerMtu =
            server->getPeerMTU(
                connHandle);

        if (peerMtu == 0)
        {
            std::cerr
                << "Could not determine peer MTU"
                << std::endl;

            return;
        }

        const auto frames =
            encodeMessageIntoFrames(
                message,
                peerMtu);

        if (frames.empty())
        {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(
                outgoingFramesMutex);

            for (
                std::size_t index = 0;
                index < frames.size();
                ++index)
            {
                outgoingFrames.push(
                    OutgoingFrame{
                        frames[index],
                        connHandle,
                        index + 1,
                        frames.size()});
            }
        }

        std::cout
            << "Queued complete BLE message: "
            << message.size()
            << " bytes"
            << std::endl;
    }
}
