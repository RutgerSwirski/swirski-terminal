

#include "ble_transport.hpp"

#include <string>

#include <iostream>

#include <NimBLEDevice.h>

namespace
{
    constexpr char SERVICE_UUID[] = "ee62dc78-0a3d-4c78-987d-23b73d5ae9d9";
    constexpr char CHARACTERISTIC_UUID[] = "0c83378b-52ed-4500-bc9e-5bb153bbd4d5";
    class ReceiveCallbacks : public NimBLECharacteristicCallbacks
    {
    public:
        void onWrite(NimBLECharacteristic *characteristic, NimBLEConnInfo &) override
        {
            const std::string message = characteristic->getValue();

            std::cout << "Received message: " << message << std::endl;
        }
    }

    ReceiveCallbacks receiveCallbacks;
};

namespace swirski::transport::ble
{
    void BleTransport::initialise()
    {
        std::cout << "Initialising BLE Server" << std::endl;

        // 1. Initialize the global NimBLE Engine
        NimBLEDevice::init("Swirski Terminal");

        // Optional: Adjust transmit power (values range from -12 to +9dBm)
        // NimBLEDevice::setPower(ESP_PWR_LVL_P9);

        NimBLEServer *server = NimBLEDevice::createServer();

        NimBLEService *service = pServer->createService(SERVICE_UUID);

        // 4. Create a BLE Characteristic with Read and Write properties
        NimBLECharacteristic *characteristic = service->createCharacteristic(
            CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

        // Set an initial value
        characteristic->setValue("Hello from NimBLE!");

        characteristic->setCallbacks(&receiveCallbacks);

        // 5. Start the service
        service->start();

        // 6. Start advertising so clients can see it
        NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();

        advertising->addServiceUUID(SERVICE_UUID);

        advertising->start();

        std::cout << "BLE Server is up and advertising!" << std::endl;
    }

    void BleTransport::update()
    {
    }

    void BleTransport::send(const std::string &message)
    {
        std::cout << "Sending ble message: " << message << std::endl;
    }

}