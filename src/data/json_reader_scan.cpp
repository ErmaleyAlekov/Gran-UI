#include "gran/data/json_reader.hpp"
#include "gran/core/i18n.hpp"

#include <cstdio>

namespace gran::data
{
    // Буфер пополняется следующим куском файла, когда прочитан до конца / The buffer is refilled with the next file chunk when read to the end
    int JsonReader::peek()
    {
        if (_position == _size)
        {
            _input.read(_buffer.data(), static_cast<std::streamsize>(_buffer.size()));
            _size = static_cast<std::size_t>(_input.gcount());
            _position = 0;
            if (_size == 0)
                return -1;
        }
        return static_cast<unsigned char>(_buffer[_position]);
    }

    // Столбец считается в символах: байты продолжения UTF-8 его не сдвигают / The column is counted in characters: the UTF-8 continuation bytes do not move it
    char JsonReader::take()
    {
        int symbol = peek();

        if (symbol < 0)
            fail(i18n::format(i18n::Message::DATA_JSON_END));
        ++_position;
        if (symbol == '\n')
        {
            ++_line;
            _column = 1;
        }
        else if ((symbol & 0xC0) != 0x80)
            ++_column;
        return static_cast<char>(symbol);
    }

    void JsonReader::skipSpace()
    {
        int symbol = peek();

        while (symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r')
        {
            take();
            symbol = peek();
        }
    }

    void JsonReader::expect(char symbol)
    {
        if (peek() != static_cast<unsigned char>(symbol))
            unexpected();
        take();
    }

    // Байт вне ASCII показывается кодом: половина символа UTF-8 в сообщении нечитаема / A non-ASCII byte is shown by its code: half a UTF-8 character is unreadable in a message
    void JsonReader::unexpected()
    {
        int symbol = peek();
        char code[8] = {};

        if (symbol < 0)
            fail(i18n::format(i18n::Message::DATA_JSON_END));
        if (symbol >= 0x20 && symbol < 0x7F)
            fail(i18n::format(i18n::Message::DATA_JSON_UNEXPECTED, {std::string(1, static_cast<char>(symbol))}));
        // peek() отдаёт байт 0…255; маска показывает это GCC (-Wformat-truncation) / peek() returns a byte 0…255; the mask shows it to GCC (-Wformat-truncation)
        std::snprintf(code, sizeof(code), "0x%02X", static_cast<unsigned>(symbol) & 0xFFu);
        fail(i18n::format(i18n::Message::DATA_JSON_UNEXPECTED, {code}));
    }
}
