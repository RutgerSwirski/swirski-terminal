#pragma once

#include <cstdint>
#include <string>

namespace swirski::services::display_text
{
    inline bool isSupported(std::uint32_t codePoint)
    {
        if (
            codePoint == '\n' ||
            codePoint == '\r' ||
            codePoint == '\t' ||
            (codePoint >= 0x20 && codePoint <= 0x7E) ||
            (codePoint >= 0xA0 && codePoint <= 0x17F))
        {
            return true;
        }

        switch (codePoint)
        {
        case 0x2013: // En dash
        case 0x2014: // Em dash
        case 0x2018:
        case 0x2019:
        case 0x201A:
        case 0x201B:
        case 0x201C:
        case 0x201D:
        case 0x201E:
        case 0x201F:
        case 0x2022: // Bullet
        case 0x2026: // Ellipsis
        case 0x20AC: // Euro
        case 0x2122: // Trademark
            return true;
        default:
            return false;
        }
    }

    inline std::size_t decode(
        const std::string &text,
        std::size_t position,
        std::uint32_t &codePoint)
    {
        const auto first =
            static_cast<unsigned char>(text[position]);

        if (first < 0x80)
        {
            codePoint = first;
            return 1;
        }

        std::size_t length = 0;
        std::uint32_t value = 0;

        if ((first & 0xE0) == 0xC0)
        {
            length = 2;
            value = first & 0x1F;
        }
        else if ((first & 0xF0) == 0xE0)
        {
            length = 3;
            value = first & 0x0F;
        }
        else if ((first & 0xF8) == 0xF0)
        {
            length = 4;
            value = first & 0x07;
        }
        else
        {
            return 0;
        }

        if (position + length > text.size())
        {
            return 0;
        }

        for (std::size_t offset = 1; offset < length; offset += 1)
        {
            const auto byte =
                static_cast<unsigned char>(text[position + offset]);

            if ((byte & 0xC0) != 0x80)
            {
                return 0;
            }

            value = (value << 6) | (byte & 0x3F);
        }

        codePoint = value;
        return length;
    }

    inline std::string normalize(const std::string &text)
    {
        std::string result;
        bool removedCharacter = false;

        for (std::size_t position = 0; position < text.size();)
        {
            std::uint32_t codePoint = 0;
            const std::size_t length = decode(text, position, codePoint);

            if (length == 0)
            {
                removedCharacter = true;
                position += 1;
                continue;
            }

            if (isSupported(codePoint))
            {
                const bool isWhitespace =
                    codePoint == ' ' ||
                    codePoint == '\n' ||
                    codePoint == '\r' ||
                    codePoint == '\t';

                if (
                    removedCharacter &&
                    !isWhitespace &&
                    !result.empty() &&
                    result.back() != ' ')
                {
                    result += ' ';
                }

                if (!(codePoint == ' ' && !result.empty() && result.back() == ' '))
                {
                    result.append(text, position, length);
                }

                removedCharacter = false;
            }
            else
            {
                removedCharacter = true;
            }

            position += length;
        }

        while (!result.empty() && result.front() == ' ')
        {
            result.erase(result.begin());
        }

        while (!result.empty() && result.back() == ' ')
        {
            result.pop_back();
        }

        return result;
    }
}
