#include "transport.hpp"

#include <string>

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

        NimBLECharacteristic *recieveCharacteristic = nullptr;

        NimBLECharacteristic *transmitCharacteristic = nullptr;

        bool connected = false;
        bool subscribed = false;
    };

}