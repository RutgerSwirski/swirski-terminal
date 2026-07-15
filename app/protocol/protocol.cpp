
#include "protocol.hpp"

#include <optional>
#include <string>

namespace swirski::protocol
{

    std::optional<std::string> handleIncomingMessage(
        const std::string &rawMessage)
    {

        std::cout << "Received BLE message in protocol: " << rawMessage << std::endl;

        // parse message eg {version: 1, type: "ping", id: "123"}

        return std::nullopt;
    }
}