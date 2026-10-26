#include "common/text_utils.hpp"

namespace chat
{
    namespace
    {
        // Пробельный символ края: пробел, табуляция, перевод строки / An edge whitespace: a space, a tab, a line break
        bool isEdgeSpace(char symbol) noexcept
        {
            return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r';
        }

        // Строчная латинская буква для ASCII (без локали) / A lowercase Latin letter for ASCII (without the locale)
        char lowerAscii(char symbol) noexcept
        {
            if (symbol >= 'A' && symbol <= 'Z')
                return static_cast<char>(symbol - 'A' + 'a');
            return symbol;
        }
    }

    // Три диапазона цифр / The three digit ranges
    int hexDigit(char symbol) noexcept
    {
        if (symbol >= '0' && symbol <= '9')
            return symbol - '0';
        if (symbol >= 'a' && symbol <= 'f')
            return symbol - 'a' + 10;
        if (symbol >= 'A' && symbol <= 'F')
            return symbol - 'A' + 10;
        return -1;
    }

    // Посимвольно после приведения к строчным / Character by character after lowering
    bool equalsIgnoreCase(std::string_view first, std::string_view second) noexcept
    {
        if (first.size() != second.size())
            return false;
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            if (lowerAscii(first[index]) != lowerAscii(second[index]))
                return false;
        }
        return true;
    }

    // Края срезаются без копирования / The edges are cut without copying
    std::string_view trimSpaces(std::string_view text) noexcept
    {
        while (!text.empty() && isEdgeSpace(text.front()))
            text.remove_prefix(1);
        while (!text.empty() && isEdgeSpace(text.back()))
            text.remove_suffix(1);
        return text;
    }
}
