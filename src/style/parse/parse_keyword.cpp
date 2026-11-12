#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

#include <string>

namespace gran::style
{
    using internal::equalsIgnoreCase;
    using internal::throwValueError;

    // Поиск слова в таблице; ошибка перечисляет все допустимые слова через запятую
    // A word lookup in the table; the error lists all allowed words separated by commas
    int parseKeyword(std::string_view text, const Keyword* table, std::size_t count)
    {
        std::string_view trimmed = trimSpaces(text);
        std::string allowed;

        for (std::size_t index = 0; index < count; ++index)
        {
            if (equalsIgnoreCase(trimmed, table[index].name))
                return table[index].value;
        }
        for (std::size_t index = 0; index < count; ++index)
        {
            if (!allowed.empty())
                allowed += ", ";
            allowed += table[index].name;
        }
        throwValueError(i18n::Message::STYLE_BAD_KEYWORD, text, allowed);
    }

    // Логическое значение в форме xs:boolean / A boolean value in the xs:boolean form
    bool parseBoolean(std::string_view text)
    {
        static const Keyword BOOLEANS[] = {{"true", 1}, {"false", 0}, {"1", 1}, {"0", 0}};

        return parseKeyword(text, BOOLEANS) == 1;
    }
}
