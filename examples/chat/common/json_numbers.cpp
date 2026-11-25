#include "common/json_internal.hpp"

#include <cmath>
#include <cstdlib>
#include <string>

namespace chat::json
{
    namespace
    {
        // Цифры подряд; false — ни одной цифры / Digits in a row; false — not a single digit
        bool skipDigits(JsonCursor& cursor) noexcept
        {
            std::size_t start = cursor.position;

            while (cursor.position < cursor.text.size() && cursor.text[cursor.position] >= '0'
                && cursor.text[cursor.position] <= '9')
                ++cursor.position;
            return cursor.position > start;
        }

        // Текущий символ равен ожидаемому (без исключения в конце текста) / The current character equals the expected one (without an exception at the end of the text)
        bool at(const JsonCursor& cursor, char expected) noexcept
        {
            return cursor.position < cursor.text.size() && cursor.text[cursor.position] == expected;
        }

        // Целая часть без ведущих нулей. / Integer part without leading zeros.
        void readInteger(JsonCursor& cursor)
        {
            if (at(cursor, '0'))
            {
                auto next = cursor.position + 1;
                if (next < cursor.text.size() && cursor.text[next] >= '0' && cursor.text[next] <= '9')
                    failAt(cursor, ChatText::JSON_BAD_NUMBER);
                ++cursor.position;
            }
            else if (!skipDigits(cursor))
                failAt(cursor, ChatText::JSON_BAD_NUMBER);
        }
    }

    /*
    ** Сначала проверка грамматики: -?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)? — strtod
    ** принял бы и «+1», и «.5», и «0x10», и «inf». Затем перевод проверенного куска strtod
    ** (в «C»-локали точка — разделитель; программа локаль не меняет).
    **
    ** First the grammar check: -?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)? — strtod
    ** would accept "+1", ".5", "0x10" and "inf" too. Then the checked piece is converted by strtod
    ** (in the "C" locale the dot is the separator; the program does not change the locale).
    */
    double parseNumber(JsonCursor& cursor)
    {
        std::size_t start = cursor.position;

        if (at(cursor, '-'))
            ++cursor.position;
        readInteger(cursor);
        if (at(cursor, '.'))
        {
            ++cursor.position;
            if (!skipDigits(cursor))
                failAt(cursor, ChatText::JSON_BAD_NUMBER);
        }
        if (at(cursor, 'e') || at(cursor, 'E'))
        {
            ++cursor.position;
            if (at(cursor, '+') || at(cursor, '-'))
                ++cursor.position;
            if (!skipDigits(cursor))
                failAt(cursor, ChatText::JSON_BAD_NUMBER);
        }
        std::string piece(cursor.text.substr(start, cursor.position - start));
        double value = std::strtod(piece.c_str(), nullptr);
        if (!std::isfinite(value))
            failAt(cursor, ChatText::JSON_BAD_NUMBER);
        return value;
    }
}
