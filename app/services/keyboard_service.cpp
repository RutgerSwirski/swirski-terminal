
#include "keyboard_service.hpp"

namespace swirski::services::keyboard_service
{
    namespace
    {
        constexpr std::size_t maxTextLength = 32;
        std::string text;
    }

    void begin(const std::string &initialText)
    {
        text = initialText.substr(0, maxTextLength);
    }

    const std::string &getText()
    {
        return text;
    }

    void addCharacter(char character)
    {
        if (text.size() < maxTextLength)
        {
            text.push_back(character);
        }
    }

    void addSpace()
    {
        addCharacter(' ');
    }

    void removeLastCharacter()
    {
        if (!text.empty())
        {
            text.pop_back();
        }
    }
}
