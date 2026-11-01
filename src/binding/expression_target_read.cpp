#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    // Текстовые атрибуты, которые можно вычислять выражением (как подстановки списков) / The text attributes that can be computed by an expression (as the list placeholders)
    std::string ui::NodeContent::* textMember(const std::string& attribute) noexcept
    {
        using C = ui::NodeContent;
        static const struct { const char* name; std::string C::* member; } TEXTS[] = {
            {"text", &C::text}, {"placeholder", &C::placeholder}, {"source", &C::source},
            {"tooltip", &C::tooltip}, {"accessibleName", &C::accessibleName}, {"dragData", &C::dragData}};

        for (const auto& text : TEXTS)
        {
            if (attribute == text.name)
                return text.member;
        }
        return nullptr;
    }

    std::string readTarget(const ui::Node& node, const std::string& attribute)
    {
        if (attribute == "text")
            return node.content.text;
        if (attribute == "checked" && node.content.checked)
            return "true";
        if (attribute == "checked")
            return "false";
        if (attribute != "value")
            return std::string();
        if (node.kind == ui::ElementKind::SLIDER || node.kind == ui::ElementKind::PROGRESS_BAR)
            return beresta::xpath::numberToString(node.content.value);
        return node.content.option;
    }

    bool editableTarget(const ui::Node& node, const std::string& attribute) noexcept
    {
        ui::ElementKind kind = node.kind;

        if (attribute == "text")
            return ui::isTextField(node);
        if (attribute == "checked")
            return kind == ui::ElementKind::CHECKBOX || kind == ui::ElementKind::SWITCH
                || kind == ui::ElementKind::RADIO_BUTTON;
        if (attribute == "value")
            return kind == ui::ElementKind::SLIDER || kind == ui::ElementKind::COMBO_BOX || kind == ui::ElementKind::TABS;
        return false;
    }

    bool booleanTarget(const std::string& attribute) noexcept
    {
        return attribute == "visible" || attribute == "enabled" || attribute == "checked";
    }
}
