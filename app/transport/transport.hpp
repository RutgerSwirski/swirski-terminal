#pragma once

#include <string>
#include <vector>

#include "notification_service.hpp"

namespace swirski::transport
{

    class Transport
    {
    public:
        virtual ~Transport() = default;

        virtual void initialise() = 0;
        virtual void update() = 0;
        virtual void send(const std::string &message) = 0;
    };

}
