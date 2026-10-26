#include "common/json_internal.hpp"
#include "common/text_utils.hpp"

#include <string>

namespace chat::json
{
    namespace
    {
        // Четыре шестнадцатеричные цифры \uXXXX → код UTF-16 / Four hex digits of \uXXXX → a UTF-16 code
        std::uint32_t readHex4(JsonCursor& cursor)
        {
            std::uint32_t code = 0;

            for (int index = 0; index < 4; ++index)
            {
                int digit = hexDigit(peekChar(cursor));
                if (digit < 0)
                    failAt(cursor, ChatText::JSON_BAD_ESCAPE);
                code = code * 16 + static_cast<std::uint32_t>(digit);
                ++cursor.position;
            }
            return code;
        }

        // Кодовая точка → 1–4 байта UTF-8 / A code point → 1–4 UTF-8 bytes
        void appendUtf8(std::string& out, std::uint32_t code)
        {
            if (code < 0x80)
                out += static_cast<char>(code);
            else if (code < 0x800)
            {
                out += static_cast<char>(0xC0 | (code >> 6));
                out += static_cast<char>(0x80 | (code & 0x3F));
            }
            else if (code < 0x10000)
            {
                out += static_cast<char>(0xE0 | (code >> 12));
                out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (code & 0x3F));
            }
            else
            {
                out += static_cast<char>(0xF0 | (code >> 18));
                out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (code & 0x3F));
            }
        }

        /*
        ** \uXXXX: старший суррогат обязан идти в паре с младшим (😀 → U+1F600);
        ** одиночный суррогат в UTF-8 непредставим — это ошибка, а не «?».
        **
        ** \uXXXX: a high surrogate must be paired with a low one (😀 → U+1F600);
        ** a lone surrogate cannot be represented in UTF-8 — it is an error, not a "?".
        */
        void readUnicodeEscape(JsonCursor& cursor, std::string& out)
        {
            std::uint32_t code = readHex4(cursor);

            if (code >= 0xDC00 && code <= 0xDFFF)
                failAt(cursor, ChatText::JSON_BAD_ESCAPE);
            if (code >= 0xD800 && code <= 0xDBFF)
            {
                expectChar(cursor, '\\');
                expectChar(cursor, 'u');
                std::uint32_t low = readHex4(cursor);
                if (low < 0xDC00 || low > 0xDFFF)
                    failAt(cursor, ChatText::JSON_BAD_ESCAPE);
                code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
            }
            appendUtf8(out, code);
        }

        // Символ после «\»: одна из восьми коротких форм или \uXXXX / The character after "\": one of the eight short forms or \uXXXX
        void readEscape(JsonCursor& cursor, std::string& out)
        {
            static const char FROM[] = "\"\\/bfnrt";
            static const char TO[] = "\"\\/\b\f\n\r\t";
            char symbol = peekChar(cursor);

            ++cursor.position;
            if (symbol == 'u')
            {
                readUnicodeEscape(cursor, out);
                return;
            }
            for (int index = 0; FROM[index] != '\0'; ++index)
            {
                if (FROM[index] == symbol)
                {
                    out += TO[index];
                    return;
                }
            }
            --cursor.position;
            failAt(cursor, ChatText::JSON_BAD_ESCAPE);
        }
    }

    // Байты UTF-8 копируются как есть; управляющие символы без экранирования запрещены (RFC 8259) / UTF-8 bytes are copied as is; unescaped control characters are forbidden (RFC 8259)
    std::string parseString(JsonCursor& cursor)
    {
        std::string out;

        expectChar(cursor, '"');
        for (char symbol = peekChar(cursor); symbol != '"'; symbol = peekChar(cursor))
        {
            if (static_cast<unsigned char>(symbol) < 0x20)
                expectChar(cursor, '"');
            ++cursor.position;
            if (symbol == '\\')
                readEscape(cursor, out);
            else
                out += symbol;
        }
        ++cursor.position;
        return out;
    }
}
