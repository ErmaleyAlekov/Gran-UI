#include "gran/data/template_text.hpp"

namespace gran::data
{
    namespace
    {
        /*
        ** Значение подстановки, начинающейся с «{» в позиции start; end — позиция после «}».
        ** false — это не подстановка (нет закрывающей скобки).
        **
        ** The value of the placeholder starting with "{" at start; end is the position after "}".
        ** false — it is not a placeholder (there is no closing brace).
        */
        bool readPlaceholder(std::string_view pattern, std::size_t start, const Record& record,
            std::string& value, std::size_t& end)
        {
            std::size_t close = pattern.find('}', start + 1);

            if (close == std::string_view::npos)
                return false;
            Record::const_iterator found = record.find(pattern.substr(start + 1, close - start - 1));
            value.clear();
            if (found != record.end())
                value = found->second;
            end = close + 1;
            return true;
        }

        // Удвоенная скобка «{{» или «}}» в позиции index / A doubled brace "{{" or "}}" at index
        bool doubledBrace(std::string_view pattern, std::size_t index) noexcept
        {
            return index + 1 < pattern.size() && pattern[index + 1] == pattern[index];
        }
    }

    // Подстановка — «{», за которой не «{» и дальше есть «}» / A placeholder is "{" not followed by "{" with a "}" further on
    bool hasPlaceholders(std::string_view pattern) noexcept
    {
        for (std::size_t index = 0; index < pattern.size(); ++index)
        {
            if (pattern[index] != '{' && pattern[index] != '}')
                continue;
            if (doubledBrace(pattern, index))
                ++index;
            else if (pattern[index] == '{' && pattern.find('}', index + 1) != std::string_view::npos)
                return true;
        }
        return false;
    }

    // Один проход слева направо: удвоенные скобки, подстановки, остальное — как есть
    // One pass from left to right: doubled braces, placeholders, the rest — as it is
    std::string expandTemplate(std::string_view pattern, const Record& record)
    {
        std::string result;
        std::string value;
        std::size_t index = 0;

        while (index < pattern.size())
        {
            char symbol = pattern[index];
            std::size_t end = 0;
            if ((symbol == '{' || symbol == '}') && doubledBrace(pattern, index))
            {
                result += symbol;
                index += 2;
            }
            else if (symbol == '{' && readPlaceholder(pattern, index, record, value, end))
            {
                result += value;
                index = end;
            }
            else
                result += pattern[index++];
        }
        return result;
    }
}
