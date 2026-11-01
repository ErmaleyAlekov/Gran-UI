#include "gran/data/record_order.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::data
{
    namespace
    {
        // Пробельные символы записи ключей / The whitespace of the key text
        constexpr std::string_view SPACES = " \t\r\n";

        // Ошибка с исходным текстом ключа / An error with the source text of the key
        [[noreturn]] void badKey(std::string_view text)
        {
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::DATA_BAD_SORT, {std::string(text)}));
        }

        // «поле[:тип]» / "field[:type]"
        void readField(std::string_view word, std::string_view source, SortKey& key)
        {
            std::size_t colon = word.find(':');
            std::string_view type;

            if (colon != std::string_view::npos)
                type = word.substr(colon + 1);
            key.field = std::string(word.substr(0, colon));
            if (key.field.empty())
                badKey(source);
            if (type == "number")
                key.type = SortType::NUMBER;
            else if (type == "natural")
                key.type = SortType::NATURAL;
            else if (!type.empty() && type != "text")
                badKey(source);
        }

        // Один ключ: слово поля и необязательное направление / One key: the field word and an optional direction
        SortKey readKey(std::string_view part)
        {
            SortKey key;
            std::size_t start = part.find_first_not_of(SPACES);

            if (start == std::string_view::npos)
                badKey(part);
            std::size_t end = part.find_first_of(SPACES, start);
            readField(part.substr(start, end - start), part, key);
            std::size_t next = part.find_first_not_of(SPACES, std::min(end, part.size()));
            if (next == std::string_view::npos)
                return key;
            std::size_t last = part.find_first_of(SPACES, next);
            std::string_view direction = part.substr(next, last - next);
            if ((direction != "asc" && direction != "desc")
                || part.find_first_not_of(SPACES, std::min(last, part.size())) != std::string_view::npos)
                badKey(part);
            key.descending = direction == "desc";
            return key;
        }
    }

    // Пустой текст — без сортировки (порядок источника) / An empty text — no sorting (the source order)
    SortKeys parseSortKeys(std::string_view text)
    {
        SortKeys keys;
        std::size_t start = 0;

        if (text.find_first_not_of(SPACES) == std::string_view::npos)
            return keys;
        while (start <= text.size())
        {
            std::size_t comma = text.find(',', start);
            if (comma == std::string_view::npos)
                comma = text.size();
            keys.push_back(readKey(text.substr(start, comma - start)));
            start = comma + 1;
        }
        return keys;
    }
}
