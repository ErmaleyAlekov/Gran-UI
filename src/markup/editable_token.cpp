#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/style/value_parsers.hpp"

namespace gran::markup::internal
{
    // Проверяем исходное значение открытого токена даже без ссылок на него.
    // Validate an exposed token's initial value even if nothing references it.
    void validateEditableToken(const beresta::Element& token)
    {
        const auto* kind = findAttribute(token, "type");
        if (!kind)
            return;
        const auto& value = *findAttribute(token, "value");
        try
        {
            if (*kind == "color")
                style::parseColor(value);
            else if (*kind != "font")
                style::parseNumber(value);
        }
        catch (const GranException& error)
        {
            throwAttributeError(token, "value", error.getMessage());
        }
        if (*kind == "font" && (value.find_first_not_of(" \t\r\n") == std::string::npos
            || value.find_first_of("{}$") != std::string::npos))
            throwAttributeError(token, "value", i18n::format(i18n::Message::OVERLAY_DENIED, {value}));
    }
}
