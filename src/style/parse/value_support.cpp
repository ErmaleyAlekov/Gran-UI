#include "gran/style/internal/value_support.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style::internal
{
    // Сообщение берётся из каталога текущего языка
    // The message is taken from the current language catalog
    void throwValueError(i18n::Message id, std::string_view text, std::string_view extra)
    {
        throw GranException(ErrorCode::STYLE_VALUE, i18n::format(id, {text, extra}));
    }

    // Число, затем '%' или идентификатор единицы, затем конец текста
    // A number, then '%' or a unit identifier, then the end of the text
    bool splitNumberUnit(std::string_view text, float& value, std::string_view& unit) noexcept
    {
        ValueScanner scanner(text);

        if (!scanner.readNumber(value))
            return false;
        std::string_view rest = scanner.rest();
        if (!rest.empty() && rest[0] == '%')
            unit = rest.substr(0, 1);
        else
            unit = scanner.readIdentifier();
        if (rest.size() != unit.size())
            return false;
        return true;
    }

    // Посимвольно с переводом A–Z в a–z / Character by character with A–Z turned into a–z
    bool equalsIgnoreCase(std::string_view first, std::string_view second) noexcept
    {
        if (first.size() != second.size())
            return false;
        for (std::size_t index = 0; index < first.size(); ++index)
        {
            char left = first[index];
            char right = second[index];
            if (left >= 'A' && left <= 'Z')
                left = static_cast<char>(left - 'A' + 'a');
            if (right >= 'A' && right <= 'Z')
                right = static_cast<char>(right - 'A' + 'a');
            if (left != right)
                return false;
        }
        return true;
    }
}
