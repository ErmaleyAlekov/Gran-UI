#include "gran/data/json_reader.hpp"
#include "gran/core/i18n.hpp"

namespace gran::data
{
    // У null значение пустое: в записи это пустое поле / null has an empty value: in a record it is an empty field
    JsonToken JsonReader::readWord()
    {
        int symbol = peek();

        _value.clear();
        if (symbol < 'a' || symbol > 'z')
            unexpected();
        while (symbol >= 'a' && symbol <= 'z')
        {
            _value.push_back(take());
            symbol = peek();
        }
        if (_value == "true" || _value == "false")
            return JsonToken::BOOLEAN;
        if (_value == "null")
        {
            _value.clear();
            return JsonToken::NUL;
        }
        fail(i18n::format(i18n::Message::DATA_JSON_UNEXPECTED, {_value}));
    }

    void JsonReader::readDigits()
    {
        int symbol = peek();

        if (symbol < '0' || symbol > '9')
            fail(i18n::format(i18n::Message::DATA_JSON_NUMBER));
        while (symbol >= '0' && symbol <= '9')
        {
            _value.push_back(take());
            symbol = peek();
        }
    }
}
