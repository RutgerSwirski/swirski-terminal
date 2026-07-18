#pragma once

#include <string>

namespace swirski::services::keyboard_service
{
    void begin(const std::string &initialText = "");
    const std::string &getText();
    void addCharacter(char character);
    void addSpace();
    void removeLastCharacter();
}
