

#include "ble_transport.hpp"

#include <functional>
#include <iostream>
#include <string>
#include <utility>

#include <NimBLEDevice.h>

#include "protocol.hpp"
#include "system_state.hpp"

namespace
{
    constexpr char SERVICE_UUID[] =
        "ee62dc78-0a3d-4c78-987d-23b73d5ae9d9";

    constexpr char RECEIVE_CHARACTERISTIC_UUID[] =
        "0c83378b-52ed-4500-bc9e-5bb153bbd4d5";

    constexpr char TRANSMIT_CHARACTERISTIC_UUID[] =
        "0c83378b-52ed-4500-bc9e-5bb153bbd4d6";

    class ServerCallbacks : public NimBLEServerCallbacks
    {
    public:
        void setConnectionHandler(
            std::function<void(bool)> handler)
        {
            connectionHandler =
                std::move(handler);
        }

        void onConnect(
            NimBLEServer *,
            NimBLEConnInfo &) override
        {
            if (connectionHandler)
            {
                connectionHandler(true);
            }
        }

        void onDisconnect(
            NimBLEServer *,
            NimBLEConnInfo &,
            int) override
        {
            if (connectionHandler)
            {
                connectionHandler(false);
            }
        }

    private:
        std::function<void(bool)> connectionHandler;
    };

    class ReceiveCallbacks : public NimBLECharacteristicCallbacks
    {
    public:
        void setMessageHandler(
            std::function<void(const std::string &)> handler)
        {
            messageHandler = std::move(handler);
        }
        void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &) override
        {
            const std::string message = characteristic->getValue();

            if (messageHandler)
            {
                messageHandler(message);
            }

            std::cout << "Received BLE message: " << message << std::endl;
        }

    private:
        std::function<void(const std::string &)> messageHandler;
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
            [this](bool connected)
            {
                std::lock_guard<std::mutex> lock(
                    connectionEventMutex);

                pendingConnectionEvent =
                    connected
                        ? ConnectionEvent::Connected
                        : ConnectionEvent::Disconnected;
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
            NIMBLE_PROPERTY::WRITE);

        transmitCharacteristic = service->createCharacteristic(
            TRANSMIT_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

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
            [this](const std::string &message)
            {
                std::lock_guard<std::mutex> lock(
                    this->incomingMessagesMutex);

                this->incomingMessages.push(message);
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
        std::optional<ConnectionEvent> connectionEvent;

        {
            std::lock_guard<std::mutex> lock(
                connectionEventMutex);

            connectionEvent = pendingConnectionEvent;

            pendingConnectionEvent.reset();
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
            << "Processing BLE message in app task: "
            << nextMessage
            << std::endl;

        const auto response = swirski::protocol::handleIncomingMessage(
            nextMessage);

        if (response)
        {
            send(*response);
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

        if (
            server == nullptr ||
            server->getConnectedCount() == 0)
        {
            std::cout
                << "No BLE client connected"
                << std::endl;

            return;
        }

        transmitCharacteristic->setValue(message);

        const bool sent =
            transmitCharacteristic->notify();

        if (!sent)
        {
            std::cerr
                << "Could not send BLE notification"
                << std::endl;

            return;
        }

        std::cout
            << "Sent BLE message: "
            << message
            << std::endl;
    }
}