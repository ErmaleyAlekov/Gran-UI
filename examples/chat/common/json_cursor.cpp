#include "common/json_internal.hpp"
#include "common/chat_errors.hpp"

#include <string>

namespace chat::json
{
    // Только четыре пробельных символа JSON (не весь isspace) / Only the four JSON whitespace characters (not all of isspace)
    void skipSpace(JsonCursor& cursor) noexcept
    {
        while (cursor.position < cursor.text.size())
        {
            char symbol = cursor.text[cursor.position];
            if (symbol != ' ' && symbol != '\t' && symbol != '\n' && symbol != '\r')
                return;
            ++cursor.position;
        }
    }

    // Чтение за концом текста — ошибка «JSON оборван» / Reading past the end of the text is the "JSON is cut off" error
    char peekChar(const JsonCursor& cursor)
    {
        if (cursor.position >= cursor.text.size())
            failAt(cursor, ChatText::JSON_UNEXPECTED_END);
        return cursor.text[cursor.position];
    }

    // Другой символ — ошибка с этим символом в тексте / Another character is an error with this character in the text
    void expectChar(JsonCursor& cursor, char expected)
    {
        char symbol = peekChar(cursor);

        if (symbol != expected)
            throw JsonException(ChatText::JSON_UNEXPECTED_CHAR,
                {std::string_view(&cursor.text[cursor.position], 1), std::to_string(cursor.position)});
        ++cursor.position;
    }

    // Предел защищает стек от «бомбы» из скобок [[[[… / The limit protects the stack from a [[[[… bracket "bomb"
    void enterNested(JsonCursor& cursor)
    {
        if (++cursor.depth > JSON_MAX_DEPTH)
            throw JsonException(ChatText::JSON_TOO_DEEP, {std::to_string(JSON_MAX_DEPTH)});
    }

    // Единственный аргумент текста — позиция / The only text argument is the position
    void failAt(const JsonCursor& cursor, ChatText id)
    {
        throw JsonException(id, {std::to_string(cursor.position)});
    }
}
