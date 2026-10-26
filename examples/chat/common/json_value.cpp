#include "common/json_value.hpp"
#include "common/json_internal.hpp"
#include "common/chat_errors.hpp"

#include <string>

namespace chat
{
    // Линейный поиск: в ответах Ollama у объектов единицы членов / A linear search: the Ollama reply objects have a handful of members
    const JsonValue* jsonMember(const JsonValue& object, std::string_view name) noexcept
    {
        if (object.type != JsonType::OBJECT)
            return nullptr;
        for (const JsonMember& member : object.members)
        {
            if (member.name == name)
                return &member.value;
        }
        return nullptr;
    }

    // Строка или запасное значение / A string or the fallback
    std::string jsonString(const JsonValue& object, std::string_view name, std::string_view fallback)
    {
        const JsonValue* value = jsonMember(object, name);

        if (value == nullptr || value->type != JsonType::STRING)
            return std::string(fallback);
        return value->text;
    }

    // Логическое значение или запасное / A boolean or the fallback
    bool jsonBoolean(const JsonValue& object, std::string_view name, bool fallback) noexcept
    {
        const JsonValue* value = jsonMember(object, name);

        if (value == nullptr || value->type != JsonType::BOOLEAN)
            return fallback;
        return value->boolean;
    }

    // Значение и только пробелы после него / A value and only spaces after it
    JsonValue parseJson(std::string_view text)
    {
        json::JsonCursor cursor{text, 0, 0};
        JsonValue value = json::parseValue(cursor);

        json::skipSpace(cursor);
        if (cursor.position < text.size())
            throw JsonException(ChatText::JSON_UNEXPECTED_CHAR,
                {text.substr(cursor.position, 1), std::to_string(cursor.position)});
        return value;
    }
}
