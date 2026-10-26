#include "gran/style/value_scanner.hpp"

#include <string>

namespace gran::style
{
    // Срез без пробелов XML по краям / The slice without XML spaces at the edges
    std::string_view trimSpaces(std::string_view text) noexcept
    {
        const std::string_view spaces(" \t\n\r");
        std::size_t begin = text.find_first_not_of(spaces);

        if (begin == std::string_view::npos)
            return std::string_view();
        std::size_t end = text.find_last_not_of(spaces);
        return text.substr(begin, end - begin + 1);
    }

    /*
    ** Разбиение по разделителю вне скобок: "rgb(1,2,3), #fff" → ["rgb(1,2,3)", "#fff"].
    ** Пустой текст даёт пустой список; пустая часть между разделителями сохраняется
    ** (её отвергнет парсер значения с понятной ошибкой).
    **
    ** Splitting by a separator outside parentheses: "rgb(1,2,3), #fff" → ["rgb(1,2,3)", "#fff"].
    ** An empty text gives an empty list; an empty part between separators is kept
    ** (the value parser rejects it with a clear error).
    */
    std::vector<std::string_view> splitTopLevel(std::string_view text, char separator)
    {
        std::vector<std::string_view> parts;
        std::size_t start = 0;
        int depth = 0;

        if (trimSpaces(text).empty())
            return parts;
        for (std::size_t index = 0; index < text.size(); ++index)
        {
            if (text[index] == '(')
                ++depth;
            else if (text[index] == ')')
                --depth;
            else if (text[index] == separator && depth == 0)
            {
                parts.push_back(trimSpaces(text.substr(start, index - start)));
                start = index + 1;
            }
        }
        parts.push_back(trimSpaces(text.substr(start)));
        return parts;
    }

    // Пробелы, табуляции и переводы строк считаются одним разделителем
    // Spaces, tabs and line breaks count as one separator
    std::vector<std::string_view> splitSpaces(std::string_view text)
    {
        std::vector<std::string_view> parts;
        std::string normalized(text);

        for (char& symbol : normalized)
        {
            if (symbol == '\t' || symbol == '\n' || symbol == '\r')
                symbol = ' ';
        }
        for (std::string_view part : splitTopLevel(normalized, ' '))
        {
            if (part.empty())
                continue;
            std::size_t offset = static_cast<std::size_t>(part.data() - normalized.data());
            parts.push_back(text.substr(offset, part.size()));
        }
        return parts;
    }
}
