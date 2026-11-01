#include "gran/data/json_reader.hpp"
#include "gran/core/i18n.hpp"

namespace gran::data
{
    namespace
    {
        // Защитный предел вложенности: файл не уронит стек разбора записей / A protective nesting limit: a file does not overflow the record parsing stack
        constexpr std::size_t MAX_DEPTH = 512;
    }

    // Родитель сразу переходит к ожиданию разделителя: его значение уже начато / The parent moves to expecting a separator at once: its value has started already
    JsonToken JsonReader::readValue()
    {
        skipSpace();
        if (!_frames.empty())
            _frames.back().expect = Expect::SEPARATOR;
        int symbol = peek();
        if (symbol == '{' || symbol == '[')
        {
            take();
            return open(symbol == '{');
        }
        if (symbol == '"')
        {
            take();
            readString();
            return JsonToken::STRING;
        }
        if (symbol == '-' || (symbol >= '0' && symbol <= '9'))
        {
            readNumber();
            return JsonToken::NUMBER;
        }
        return readWord();
    }

    /*
    ** Объект: после имени и двоеточия — значение; после значения — запятая или «}»; в начале — имя или «}».
    **
    ** An object: after a name and a colon — a value; after a value — a comma or "}"; at the start — a name or "}".
    */
    JsonToken JsonReader::nextInObject(Frame& frame)
    {
        skipSpace();
        if (frame.expect == Expect::VALUE)
            return readValue();
        if (frame.expect != Expect::NEXT && peek() == '}')
        {
            take();
            return close(JsonToken::END_OBJECT);
        }
        if (frame.expect == Expect::SEPARATOR)
        {
            expect(',');
            skipSpace();
        }
        expect('"');
        readString();
        skipSpace();
        expect(':');
        frame.expect = Expect::VALUE;
        return JsonToken::NAME;
    }

    // Массив: в начале — значение или «]», после значения — запятая или «]» / An array: at the start — a value or "]", after a value — a comma or "]"
    JsonToken JsonReader::nextInArray(Frame& frame)
    {
        skipSpace();
        if (peek() == ']' && frame.expect != Expect::NEXT)
        {
            take();
            return close(JsonToken::END_ARRAY);
        }
        if (frame.expect == Expect::SEPARATOR)
        {
            expect(',');
            frame.expect = Expect::NEXT;
        }
        return readValue();
    }

    JsonToken JsonReader::close(JsonToken token)
    {
        _frames.pop_back();
        return token;
    }

    JsonToken JsonReader::open(bool object)
    {
        if (_frames.size() >= MAX_DEPTH)
            fail(i18n::format(i18n::Message::DATA_JSON_DEPTH, {std::to_string(MAX_DEPTH)}));
        _frames.push_back(Frame{object, Expect::FIRST});
        if (object)
            return JsonToken::BEGIN_OBJECT;
        return JsonToken::BEGIN_ARRAY;
    }
}
