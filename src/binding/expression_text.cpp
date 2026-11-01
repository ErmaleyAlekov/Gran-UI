#include "gran/binding/expression_text.hpp"

namespace gran::binding
{
    namespace
    {
        /*
        ** Закрывающая «}» выражения, начатого перед start. Строковые литералы XPath ('…' и "…")
        ** пропускаются целиком: «}» внутри строки выражение не закрывает. npos — не закрыто.
        **
        ** The closing "}" of the expression started before start. The XPath string literals ('…' and "…")
        ** are skipped whole: a "}" inside a string does not close the expression. npos — not closed.
        */
        std::size_t findClose(std::string_view value, std::size_t start) noexcept
        {
            std::size_t position = start;

            while (position < value.size())
            {
                char symbol = value[position];
                if (symbol == '}')
                    return position;
                if (symbol == '\'' || symbol == '"')
                {
                    position = value.find(symbol, position + 1);
                    if (position == std::string_view::npos)
                        return position;
                }
                ++position;
            }
            return std::string_view::npos;
        }
    }

    // Байты ≥ 0x80 — части букв UTF-8 (как NameChar у XML) / Bytes ≥ 0x80 are parts of UTF-8 letters (as an XML NameChar)
    bool isFieldName(std::string_view text) noexcept
    {
        if (text.empty())
            return false;
        for (std::size_t index = 0; index < text.size(); ++index)
        {
            unsigned char symbol = static_cast<unsigned char>(text[index]);
            bool letter = (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z') || symbol == '_'
                || symbol >= 0x80;
            bool tail = (symbol >= '0' && symbol <= '9') || symbol == '-' || symbol == '.';
            if (!letter && (index == 0 || !tail))
                return false;
        }
        return true;
    }

    bool isWholeExpression(std::string_view value) noexcept
    {
        if (value.size() < 2 || value.front() != '{' || value[1] == '{')
            return false;
        return findClose(value, 1) == value.size() - 1;
    }

    bool hasExpressions(std::string_view value) noexcept
    {
        if (value.find('{') == std::string_view::npos)
            return false;
        for (const TextPart& part : splitText(value))
        {
            if (part.expression && !isFieldName(part.text))
                return true;
        }
        return false;
    }

    /*
    ** Как шаблоны полей списков: «{{» и «}}» — сами скобки, «{…}» — выражение. Соседние литералы
    ** склеиваются, поэтому текст без выражений даёт ровно одну часть.
    **
    ** As the list field templates: "{{" and "}}" are the brackets themselves, "{…}" is an expression. Adjacent literals
    ** are joined, so a text without expressions gives exactly one part.
    */
    std::vector<TextPart> splitText(std::string_view value)
    {
        std::vector<TextPart> parts(1);
        std::size_t position = 0;

        while (position < value.size())
        {
            char symbol = value[position];
            std::size_t close = std::string_view::npos;
            if (symbol == '{' && position + 1 < value.size() && value[position + 1] != '{')
                close = findClose(value, position + 1);
            if (close != std::string_view::npos)
            {
                parts.push_back(TextPart{std::string(value.substr(position + 1, close - position - 1)), true});
                parts.push_back(TextPart{});
                position = close + 1;
                continue;
            }
            // Удвоенная скобка даёт одну / A doubled bracket gives one
            if ((symbol == '{' || symbol == '}') && position + 1 < value.size() && value[position + 1] == symbol)
                ++position;
            parts.back().text.push_back(symbol);
            ++position;
        }
        return parts;
    }
}
