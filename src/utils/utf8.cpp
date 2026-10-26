#include "gran/utils/utf8.hpp"

namespace gran
{
    namespace
    {
        // Байт продолжения 10xxxxxx / A continuation byte 10xxxxxx
        bool isContinuation(unsigned char byte) noexcept
        {
            return (byte & 0xC0) == 0x80;
        }

        // Длина последовательности по первому байту и его значащие биты; 0 — недопустимый байт
        // The sequence length by the first byte and its significant bits; 0 — an invalid byte
        std::size_t sequenceLength(unsigned char first, char32_t& bits) noexcept
        {
            if (first < 0x80)
            {
                bits = first;
                return 1;
            }
            if (first >= 0xC2 && first <= 0xDF)
            {
                bits = first & 0x1F;
                return 2;
            }
            if (first >= 0xE0 && first <= 0xEF)
            {
                bits = first & 0x0F;
                return 3;
            }
            if (first >= 0xF0 && first <= 0xF4)
            {
                bits = first & 0x07;
                return 4;
            }
            return 0;
        }
    }

    // Проверяются: длина, байты продолжения, избыточная форма, суррогаты, предел U+10FFFF
    // Checked: the length, the continuation bytes, the overlong form, surrogates, the U+10FFFF limit
    char32_t decodeUtf8(std::string_view text, std::size_t& position) noexcept
    {
        static const char32_t MINIMUM[5] = {0, 0, 0x80, 0x800, 0x10000};
        char32_t symbol = 0;
        std::size_t length = sequenceLength(static_cast<unsigned char>(text[position]), symbol);

        if (length == 0 || position + length > text.size())
        {
            ++position;
            return REPLACEMENT_CHARACTER;
        }
        for (std::size_t index = 1; index < length; ++index)
        {
            unsigned char byte = static_cast<unsigned char>(text[position + index]);
            if (!isContinuation(byte))
            {
                ++position;
                return REPLACEMENT_CHARACTER;
            }
            symbol = (symbol << 6) | (byte & 0x3F);
        }
        if (symbol < MINIMUM[length] || symbol > 0x10FFFF || (symbol >= 0xD800 && symbol <= 0xDFFF))
        {
            ++position;
            return REPLACEMENT_CHARACTER;
        }
        position += length;
        return symbol;
    }

    // Кодирование 1–4 байтами / Encoding with 1–4 bytes
    void appendUtf8(std::string& text, char32_t symbol)
    {
        if (symbol > 0x10FFFF || (symbol >= 0xD800 && symbol <= 0xDFFF))
            symbol = REPLACEMENT_CHARACTER;
        if (symbol < 0x80)
            text += static_cast<char>(symbol);
        else if (symbol < 0x800)
        {
            text += static_cast<char>(0xC0 | (symbol >> 6));
            text += static_cast<char>(0x80 | (symbol & 0x3F));
        }
        else if (symbol < 0x10000)
        {
            text += static_cast<char>(0xE0 | (symbol >> 12));
            text += static_cast<char>(0x80 | ((symbol >> 6) & 0x3F));
            text += static_cast<char>(0x80 | (symbol & 0x3F));
        }
        else
        {
            text += static_cast<char>(0xF0 | (symbol >> 18));
            text += static_cast<char>(0x80 | ((symbol >> 12) & 0x3F));
            text += static_cast<char>(0x80 | ((symbol >> 6) & 0x3F));
            text += static_cast<char>(0x80 | (symbol & 0x3F));
        }
    }

    // Подсчёт тем же декодером: испорченный байт — один символ / Counting with the same decoder: a broken byte is one character
    std::size_t countUtf8(std::string_view text) noexcept
    {
        std::size_t count = 0;

        for (std::size_t position = 0; position < text.size(); ++count)
            decodeUtf8(text, position);
        return count;
    }
}
