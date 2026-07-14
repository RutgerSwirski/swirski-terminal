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
    };

}