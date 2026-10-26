#include "common/json_internal.hpp"
#include "common/chat_errors.hpp"

#include <string>
#include <utility>

namespace chat::json
{
    namespace
    {
        // Слово true / false / null целиком, иначе — ошибка на первом несовпавшем символе / The whole true / false / null word, otherwise an error at the first mismatching character
        void expectWord(JsonCursor& cursor, std::string_view word)
        {
            for (char symbol : word)
                expectChar(cursor, symbol);
        }

        // [ значение, значение ] — пустой массив допустим, висячая запятая — нет / [ value, value ] — an empty array is allowed, a trailing comma is not
        JsonValue parseArray(JsonCursor& cursor)
        {
            JsonValue array;

            array.type = JsonType::ARRAY;
            enterNested(cursor);
            expectChar(cursor, '[');
            skipSpace(cursor);
            if (peekChar(cursor) != ']')
            {
                array.items.push_back(parseValue(cursor));
                skipSpace(cursor);
                while (peekChar(cursor) == ',')
                {
                    ++cursor.position;
                    array.items.push_back(parseValue(cursor));
                    skipSpace(cursor);
                }
            }
            expectChar(cursor, ']');
            --cursor.depth;
            return array;
        }

        // { "имя": значение, ... } — член читается вместе с двоеточием / { "name": value, ... } — a member is read together with the colon
        JsonValue parseObject(JsonCursor& cursor)
        {
            JsonValue object;

            object.type = JsonType::OBJECT;
            enterNested(cursor);
            expectChar(cursor, '{');
            skipSpace(cursor);
            bool more = peekChar(cursor) != '}';
            while (more)
            {
                skipSpace(cursor);
                JsonMember member;
                member.name = parseString(cursor);
                skipSpace(cursor);
                expectChar(cursor, ':');
                member.value = parseValue(cursor);
                object.members.push_back(std::move(member));
                skipSpace(cursor);
                more = peekChar(cursor) == ',';
                if (more)
                    ++cursor.position;
            }
            expectChar(cursor, '}');
            --cursor.depth;
            return object;
        }

        // true / false / null; любой другой символ здесь — ошибка в expectWord / true / false / null; any other character here is an error in expectWord
        JsonValue parseLiteral(JsonCursor& cursor)
        {
            JsonValue value;
            char symbol = peekChar(cursor);

            if (symbol == 'n')
            {
                expectWord(cursor, "null");
                return value;
            }
            value.type = JsonType::BOOLEAN;
            value.boolean = symbol == 't';
            if (value.boolean)
                expectWord(cursor, "true");
            else
                expectWord(cursor, "false");
            return value;
        }
    }

    // Вид значения — по первому символу (паттерн Interpreter для грамматики JSON) / The value kind — by the first character (the Interpreter pattern for the JSON grammar)
    JsonValue parseValue(JsonCursor& cursor)
    {
        JsonValue value;

        skipSpace(cursor);
        char symbol = peekChar(cursor);
        if (symbol == '{')
            return parseObject(cursor);
        if (symbol == '[')
            return parseArray(cursor);
        if (symbol == 't' || symbol == 'f' || symbol == 'n')
            return parseLiteral(cursor);
        if (symbol == '"')
        {
            value.type = JsonType::STRING;
            value.text = parseString(cursor);
            return value;
        }
        value.type = JsonType::NUMBER;
        value.number = parseNumber(cursor);
        return value;
    }
}

