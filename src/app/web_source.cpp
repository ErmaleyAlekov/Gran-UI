#include "gran/app/internal/web_slots.hpp"

#include <filesystem>

namespace gran::app
{
    namespace
    {
        // Есть ли у адреса схема: буквы, цифры, «+-.» до первого «:» (одна буква — диск Windows, не схема)
        // Whether the address has a scheme: letters, digits, "+-." before the first ":" (one letter is a Windows drive, not a scheme)
        bool hasScheme(const std::string& source) noexcept
        {
            std::size_t colon = source.find(':');

            if (colon == std::string::npos || colon < 2)
                return false;
            for (std::size_t index = 0; index < colon; ++index)
            {
                char symbol = source[index];
                bool letter = (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z');
                bool other = (symbol >= '0' && symbol <= '9') || symbol == '+' || symbol == '-' || symbol == '.';
                if (!letter && !(index > 0 && other))
                    return false;
            }
            return true;
        }

        // Байты пути, допустимые в URL как есть; остальные (пробел, %, #, ?, не-ASCII) — %XX
        // The path bytes allowed in a URL as they are; the others (space, %, #, ?, non-ASCII) — %XX
        bool plainByte(unsigned char symbol) noexcept
        {
            if ((symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z') || (symbol >= '0' && symbol <= '9'))
                return true;
            return symbol == '/' || symbol == ':' || symbol == '-' || symbol == '_' || symbol == '.' || symbol == '~';
        }
    }

    // Путь собирается в UTF-8 (generic_u8string), затем экранируется побайтно / The path is assembled in UTF-8 (generic_u8string), then escaped byte by byte
    std::string webSourceUrl(const std::string& source, const std::string& baseDirectory)
    {
        static const char HEX[] = "0123456789ABCDEF";
        std::string url = "file:///";

        if (hasScheme(source))
            return source;
        std::filesystem::path path = std::filesystem::u8path(source);
        if (path.is_relative())
            path = std::filesystem::absolute(std::filesystem::u8path(baseDirectory) / path);
        std::string utf8 = path.lexically_normal().generic_u8string();
        if (!utf8.empty() && utf8.front() == '/')
            utf8.erase(0, 1);
        for (unsigned char symbol : utf8)
        {
            if (plainByte(symbol))
                url += static_cast<char>(symbol);
            else
                url.append(1, '%').append(1, HEX[symbol >> 4]).append(1, HEX[symbol & 15]);
        }
        return url;
    }
}
