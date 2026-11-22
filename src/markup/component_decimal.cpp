#include "gran/markup/internal/components.hpp"
#include "gran/core/i18n.hpp"

namespace gran::markup::internal
{
    // Десятичный литерал схемы без унарного плюса для XPath / Schema decimal without XPath's unsupported unary plus
    std::string decimalLiteral(const ComponentParam& param, std::string text, const Substitution& substitution)
    {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first != std::string::npos)
        {
            text = text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
            std::size_t start = 0;
            if (text.front() == '-' || text.front() == '+') start = 1;
            const auto dot = text.find('.');
            if (text.find_first_of("0123456789") != std::string::npos
                && text.find_first_not_of("0123456789.", start) == std::string::npos
                && (dot == std::string::npos || text.find('.', dot + 1) == std::string::npos))
            {
                if (text.front() == '+') text.erase(0, 1);
                return text;
            }
        }
        throwAttributeError(*substitution.spec->body, param.name, i18n::format(i18n::Message::STYLE_BAD_NUMBER, {text}));
    }
}
