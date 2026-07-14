

#include "ble_transport.hpp"

#include <string>

#include <iostream>

namespace swirski::transport::ble
{
    void BleTransport::initialise()
    {
        std::cout << "Initialising ble transport" << std::endl;
    }

    void BleTransport::update()
    {
        std::cout << "Updating ble transport" << std::endl;
    }

    void BleTransport::send(const std::string &message)
    {
        std::cout << "Sending ble message: " << message << std::endl;
    }

}