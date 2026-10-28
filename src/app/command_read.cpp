#include "gran/app/commands.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <iomanip>
#include <locale>
#include <sstream>

namespace gran::app
{
    // Классическая локаль: точка, а не запятая, даже после setlocale в программе; 7 значащих цифр float.
    // The classic locale: a dot, not a comma, even after setlocale in the program; 7 significant float digits.
    static std::string numberText(float value)
    {
        std::ostringstream out;

        out.imbue(std::locale::classic());
        out << std::setprecision(7) << value;
        return out.str();
    }

    // Строковое представление едино для всех языков; числа не зависят от локали.
    // String representation is shared by all languages; numbers are locale independent.
    std::string readProperty(const ui::Node& node, const std::string& property)
    {
        if (property == "text") return node.content.text;
        if (property == "option") return node.content.option;
        if (property == "kind") return ui::elementName(node.kind);
        if (property == "value") return numberText(node.content.value);
        if (property == "checked")
        {
            if (node.content.checked) return "true";
            return "false";
        }
        if (property == "visible" || property == "enabled")
        {
            bool enabled = node.enabled;
            if (property == "visible") enabled = node.visible;
            if (enabled) return "true";
            return "false";
        }
        if (property == "style")
        {
            if (node.rule != nullptr) return node.rule->name;
            return "";
        }
        throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::CAPI_PROPERTY, {property}));
    }
}
