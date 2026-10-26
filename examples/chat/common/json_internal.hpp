#ifndef CHAT_COMMON_JSON_INTERNAL_HPP
#define CHAT_COMMON_JSON_INTERNAL_HPP

#include "common/chat_texts.hpp"
#include "common/json_value.hpp"

#include <cstddef>
#include <string_view>

namespace chat::json
{
    // Курсор разбора: текст, позиция, текущая вложенность / The parse cursor: the text, the position, the current nesting
    struct JsonCursor
    {
        std::string_view text;
        std::size_t position = 0;
        int depth = 0;
    };

    // ---- json_cursor.cpp ----

    // Пропуск пробелов JSON (пробел, \t, \n, \r) / Skipping the JSON spaces (space, \t, \n, \r)
    void skipSpace(JsonCursor& cursor) noexcept;

    // Текущий символ; конец текста — JsonException / The current character; the end of the text — JsonException
    char peekChar(const JsonCursor& cursor);

    // Ожидаемый символ: есть — шаг вперёд, нет — JsonException / The expected character: present — a step forward, absent — JsonException
    void expectChar(JsonCursor& cursor, char expected);

    // Вход во вложенный массив или объект; глубже JSON_MAX_DEPTH — JsonException / Entering a nested array or object; deeper than JSON_MAX_DEPTH — JsonException
    void enterNested(JsonCursor& cursor);

    // Ошибка с позицией курсора (для текстов вида «… (позиция {0})») / An error with the cursor position (for "… (position {0})" texts)
    [[noreturn]] void failAt(const JsonCursor& cursor, ChatText id);

    // ---- json_parser.cpp ----

    // Любое значение с текущей позиции (пробелы перед ним пропускаются) / Any value from the current position (the spaces before it are skipped)
    JsonValue parseValue(JsonCursor& cursor);

    // ---- json_strings.cpp ----

    // Строка в кавычках → UTF-8 / A quoted string → UTF-8
    std::string parseString(JsonCursor& cursor);

    // ---- json_numbers.cpp ----

    // Число по грамматике JSON (без ведущих нулей, «+», «.5») / A number by the JSON grammar (without leading zeros, "+", ".5")
    double parseNumber(JsonCursor& cursor);
}

#endif
