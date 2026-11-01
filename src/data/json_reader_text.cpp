#include "gran/data/json_reader.hpp"
#include "gran/core/i18n.hpp"
#include "gran/utils/utf8.hpp"

#include <cstring>

namespace gran::data
{
    // Управляющие символы внутри строки JSON запрещены — их пишут через \n, \t, \uXXXX / Control characters inside a JSON string are forbidden — they are written as \n, \t, \uXXXX
    void JsonReader::readString()
    {
        _value.clear();
        for (;;)
        {
            char symbol = take();
            if (symbol == '"')
                return;
            if (symbol == '\\')
                readEscape();
            else if (static_cast<unsigned char>(symbol) < 0x20)
                fail(i18n::format(i18n::Message::DATA_JSON_ESCAPE));
            else
                _value.push_back(symbol);
        }
    }

    /*
    ** Простые замены — по таблице; \uXXXX — символ, а пара суррогатов 😀 — один символ
    ** вне основной плоскости. Одинокий суррогат — ошибка: в UTF-8 его записать нельзя.
    **
    ** The simple replacements go by a table; \uXXXX is a character, and a surrogate pair 😀 is one character
    ** outside the basic plane. A lone surrogate is an error: it cannot be written in UTF-8.
    */
    void JsonReader::readEscape()
    {
        static const char FROM[] = "\"\\/bfnrt";
        static const char TO[] = "\"\\/\b\f\n\r\t";
        char symbol = take();
        const char* found = std::strchr(FROM, symbol);

        if (symbol != '\0' && symbol != 'u' && found != nullptr)
        {
            _value.push_back(TO[found - FROM]);
            return;
        }
        if (symbol != 'u')
            fail(i18n::format(i18n::Message::DATA_JSON_ESCAPE));
        char32_t code = readHex();
        if (code >= 0xD800 && code <= 0xDBFF)
        {
            expect('\\');
            expect('u');
            char32_t low = readHex();
            if (low < 0xDC00 || low > 0xDFFF)
                fail(i18n::format(i18n::Message::DATA_JSON_ESCAPE));
            code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
        }
        else if (code >= 0xDC00 && code <= 0xDFFF)
            fail(i18n::format(i18n::Message::DATA_JSON_ESCAPE));
        appendUtf8(_value, code);
    }

    char32_t JsonReader::readHex()
    {
        char32_t code = 0;

        for (int digit = 0; digit < 4; ++digit)
        {
            char symbol = take();
            code <<= 4;
            if (symbol >= '0' && symbol <= '9')
                code |= static_cast<char32_t>(symbol - '0');
            else if (symbol >= 'a' && symbol <= 'f')
                code |= static_cast<char32_t>(symbol - 'a' + 10);
            else if (symbol >= 'A' && symbol <= 'F')
                code |= static_cast<char32_t>(symbol - 'A' + 10);
            else
                fail(i18n::format(i18n::Message::DATA_JSON_ESCAPE));
        }
        return code;
    }

    // -?(0|[1-9][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]+)? — текст числа сохраняется как есть / The number text is kept as it is
    void JsonReader::readNumber()
    {
        _value.clear();
        if (peek() == '-')
            _value.push_back(take());
        if (peek() == '0')
            _value.push_back(take());
        else
            readDigits();
        if (peek() == '.')
        {
            _value.push_back(take());
            readDigits();
        }
        if (peek() == 'e' || peek() == 'E')
        {
            _value.push_back(take());
            if (peek() == '+' || peek() == '-')
                _value.push_back(take());
            readDigits();
        }
    }
}
