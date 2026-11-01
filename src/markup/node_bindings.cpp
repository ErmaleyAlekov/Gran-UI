#include "gran/markup/internal/build_context.hpp"
#include "gran/binding/expression_text.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/style/property.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        using C = ui::NodeContent;

        // Текстовые атрибуты с подстановками и выражениями (value пункта — ниже, отдельно) / The text attributes with placeholders and expressions (an item value is below, separately)
        const struct { const char* name; std::string C::* member; } TEXTS[] = {
            {"text", &C::text}, {"placeholder", &C::placeholder}, {"source", &C::source},
            {"tooltip", &C::tooltip}, {"accessibleName", &C::accessibleName}, {"dragData", &C::dragData}};

        // У этих видов value — значение выбора или число; у прочих (Input…) value — это текст / For these kinds value is a choice value or a number; for the others (Input…) value is the text
        bool holdsValue(ui::ElementKind kind) noexcept
        {
            return kind == ui::ElementKind::SLIDER || kind == ui::ElementKind::PROGRESS_BAR
                || kind == ui::ElementKind::MENU_ITEM || kind == ui::ElementKind::COMBO_BOX
                || kind == ui::ElementKind::RADIO_BUTTON || kind == ui::ElementKind::TABS || kind == ui::ElementKind::TAB;
        }

        // Ошибка синтаксиса — с позицией элемента и именем атрибута / A syntax error — with the element position and the attribute name
        void addBinding(const beresta::Element& element, ui::Node& node, const std::string& name, const std::string& source)
        {
            try
            {
                binding::checkExpressionText(source);
            }
            catch (const GranException& error)
            {
                throwAttributeError(element, name, error.getMessage());
            }
            node.content.bindings.push_back(ui::AttributeBinding{name, source});
        }
    }

    /*
    ** Нетекстовый атрибут целиком из выражения: visible, enabled, checked, value и свойства стиля.
    ** value пункта с одними {поле} остаётся прежней подстановкой списков (ComboBox подбирает подпись
    ** по значениям сразу после построения пунктов).
    **
    ** A non-text attribute made of an expression as a whole: visible, enabled, checked, value and the style properties.
    ** An item value with only {field} placeholders stays the former list substitution (a ComboBox picks the label
    ** by the values right after the items are built).
    */
    bool readBindingAttribute(const beresta::Element& element, const beresta::Attribute& attribute, ui::Node& node)
    {
        const std::string& name = attribute.name;
        bool value = name == "value";

        if (!binding::isWholeExpression(attribute.value))
            return false;
        if (value && holdsValue(node.kind) && node.kind != ui::ElementKind::SLIDER
            && node.kind != ui::ElementKind::PROGRESS_BAR && !binding::hasExpressions(attribute.value))
            return false;
        if (value && !holdsValue(node.kind))
        {
            addBinding(element, node, "text", attribute.value);
            return true;
        }
        if (!value && name != "visible" && name != "enabled" && name != "checked" && style::findProperty(name) == nullptr)
            return false;
        addBinding(element, node, name, attribute.value);
        return true;
    }

    /*
    ** Текстовые поля с выражениями (не только {поле}) уходят в привязки, а само поле пустеет до
    ** первого вычисления: так подстановка списков не тронет выражение. HTML <WebView> не разбирается —
    ** в нём фигурные скобки обычны (CSS, JavaScript); текст Text с участками — склейка участков.
    **
    ** The text fields with expressions (not only {field}) go into the bindings, and the field itself stays empty until
    ** the first evaluation: this way the list substitution does not touch the expression. The <WebView> HTML is not parsed —
    ** curly brackets are usual in it (CSS, JavaScript); the text of a Text with runs is the joined runs.
    */
    void readTextBindings(const beresta::Element& element, ui::Node& node)
    {
        if (node.kind == ui::ElementKind::WEB_VIEW)
            return;
        for (const auto& text : TEXTS)
        {
            std::string& field = node.content.*text.member;
            if (text.member == &C::text && ui::hasSpans(node))
                continue;
            if (!binding::hasExpressions(field))
                continue;
            addBinding(element, node, text.name, field);
            field.clear();
        }
        if (holdsValue(node.kind) && binding::hasExpressions(node.content.option))
        {
            addBinding(element, node, "value", node.content.option);
            node.content.option.clear();
        }
    }
}
