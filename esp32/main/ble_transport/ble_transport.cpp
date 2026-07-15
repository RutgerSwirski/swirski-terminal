

#include "ble_transport.hpp"

#include <string>

#include <iostream>

#include <NimBLEDevice.h>

namespace
{
    constexpr char SERVICE_UUID[] =
        "ee62dc78-0a3d-4c78-987d-23b73d5ae9d9";

    constexpr char RECEIVE_CHARACTERISTIC_UUID[] =
        "0c83378b-52ed-4500-bc9e-5bb153bbd4d5";

    constexpr char TRANSMIT_CHARACTERISTIC_UUID[] =
        "0c83378b-52ed-4500-bc9e-5bb153bbd4d6";
    class ReceiveCallbacks : public NimBLECharacteristicCallbacks
    {
    public:
        void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &) override
        {
            const std::string message = characteristic->getValue();

            std::cout << "Received BLE message: " << message << std::endl;
        }
    };

    ReceiveCallbacks receiveCallbacks;
}

namespace swirski::transport::ble
{
    void BleTransport::initialise()
    {
        std::cout << "Initialising BLE Server" << std::endl;

        const bool initialised = NimBLEDevice::init("Swirski Terminal");

        if (!initialised)
        {
            std::cerr
                << "Could not initialise NimBLE"
                << std::endl;

            return;
        }

        // Optional: Adjust transmit power (values range from -12 to +9dBm)
        // NimBLEDevice::setPower(ESP_PWR_LVL_P9);

        server = NimBLEDevice::createServer();

        if (server == nullptr)
        {
            std::cerr
                << "Could not create BLE server"
                << std::endl;

            return;
        }

        server->advertiseOnDisconnect(true);

        NimBLEService *service = server->createService(SERVICE_UUID);

        if (service == nullptr)
        {
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
            std::cerr
                << "Could not create BLE characteristic"
                << std::endl;
            return;
        }

        receiveCharacteristic->setCallbacks(&receiveCallbacks);

        transmitCharacteristic->setValue("Hello World");

        const bool serverStarted = server->start();

        if (!serverStarted)
        {
            std::cerr
                << "Could not start BLE server"
                << std::endl;

            return;
        }

        NimBLEAdvertising *advertising = server->getAdvertising();

        if (advertising == nullptr)
        {
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
            std::cerr
                << "Could not add service UUID to BLE advertising"
                << std::endl;
            return;
        }

        const bool nameSet = advertising->setName("Swirski Terminal");

        if (!nameSet)
        {
            std::cerr
                << "Could not set name to BLE advertising"
                << std::endl;
            return;
        }

        const bool advertisingStarted = advertising->start();

        if (!advertisingStarted)
        {
            std::cerr
                << "Could not start BLE advertising"
                << std::endl;

            return;
        }

        std::cout << "BLE Server started and advertising" << std::endl;
    }

    void BleTransport::update()
    {
    }

    void BleTransport::send(const std::string &message)
    {
        std::cout << "Sending ble message: " << message << std::endl;
    }
}