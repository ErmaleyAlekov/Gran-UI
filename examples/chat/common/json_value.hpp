#ifndef CHAT_COMMON_JSON_VALUE_HPP
#define CHAT_COMMON_JSON_VALUE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace chat
{
    // Вид значения JSON / The JSON value kind
    enum class JsonType : std::uint8_t { NULL_VALUE, BOOLEAN, NUMBER, STRING, ARRAY, OBJECT };

    // Пара «имя — значение» объекта (определена ниже) / An object "name — value" pair (defined below)
    struct JsonMember;

    /*
    ** Значение JSON — плоская структура без иерархии классов (как ui::Node): у каждого вида
    ** свои поля, остальные пусты. Порядок членов объекта сохраняется, как в тексте.
    **
    ** A JSON value — a flat structure without a class hierarchy (as ui::Node): each kind
    ** uses its own fields, the others stay empty. The object member order is kept as in the text.
    */
    struct JsonValue
    {
        JsonType type = JsonType::NULL_VALUE;
        bool boolean = false;
        double number = 0.0;
        std::string text;
        std::vector<JsonValue> items;
        std::vector<JsonMember> members;
    };

    struct JsonMember
    {
        std::string name;
        JsonValue value;
    };

    // ---- json_value.cpp ----

    // Член объекта по имени (первый с таким именем); не объект или нет члена — nullptr / An object member by name (the first one); not an object or no member — nullptr
    const JsonValue* jsonMember(const JsonValue& object, std::string_view name) noexcept;

    // Строка члена или запасное значение, если члена нет или он не строка / A member string or the fallback if there is no member or it is not a string
    std::string jsonString(const JsonValue& object, std::string_view name, std::string_view fallback = {});

    // Логическое значение члена или запасное / A member boolean or the fallback
    bool jsonBoolean(const JsonValue& object, std::string_view name, bool fallback = false) noexcept;

    /*
    ** Разбор текста JSON (RFC 8259) целиком: после значения допустимы только пробелы.
    ** Строки — в UTF-8 (\uXXXX и суррогатные пары переводятся); вложенность ограничена
    ** JSON_MAX_DEPTH. Ошибка — JsonException с позицией (байт от начала, с 0).
    **
    ** Parsing a whole JSON text (RFC 8259): only spaces may follow the value.
    ** Strings are in UTF-8 (\uXXXX and surrogate pairs are converted); the nesting is limited by
    ** JSON_MAX_DEPTH. An error is JsonException with the position (a byte from the start, from 0).
    */
    JsonValue parseJson(std::string_view text);

    // Предел вложенности массивов и объектов / The array and object nesting limit
    constexpr int JSON_MAX_DEPTH = 64;

    // ---- json_writer.cpp ----

    // Строка в кавычках с экранированием (", \, управляющие символы) дописывается в out / A quoted string with escaping (", \, control characters) is appended to out
    void writeJsonString(std::string& out, std::string_view text);
}

#endif
