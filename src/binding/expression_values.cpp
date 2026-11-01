#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    namespace
    {
        // Число у ползунка и индикатора, значение выбора у прочих (ComboBox, Tabs, RadioButton…) / A number for a slider and a progress bar, the choice value for the others (ComboBox, Tabs, RadioButton…)
        beresta::xpath::XPathValue valueProperty(const ui::Node& node)
        {
            if (node.kind == ui::ElementKind::SLIDER || node.kind == ui::ElementKind::PROGRESS_BAR)
                return beresta::xpath::XPathValue::fromNumber(node.content.value);
            return beresta::xpath::XPathValue::fromString(node.content.option);
        }
    }

    /*
    ** id ищется в окне самой привязки (у разных окон могут быть одинаковые id) и запоминается до конца
    ** прохода update: сотня привязок к одному полю обходит дерево один раз.
    **
    ** An id is looked up in the window of the binding itself (different windows may have equal ids) and remembered until the end
    ** of the update pass: a hundred bindings to one field walk the tree once.
    */
    const ui::Node* propertyNode(EvaluationScope& scope, const ExpressionBinding& binding, const std::string& base)
    {
        ui::Node* root = binding.node;

        if (base == "self")
            return binding.node;
        if (base == "parent")
            return binding.node->parent;
        while (root->parent != nullptr)
            root = root->parent;
        std::pair<const ui::Node*, std::string> key(root, base);
        auto cached = scope.found.find(key);
        if (cached != scope.found.end())
            return cached->second;
        ui::Node* node = ui::findNode(*root, base);
        scope.found.emplace(std::move(key), node);
        return node;
    }

    // Размеры — рамка последней компоновки в логических пикселях / The sizes are the last layout frame in logical pixels
    beresta::xpath::XPathValue propertyValue(const ui::Node* node, const std::string& property)
    {
        using beresta::xpath::XPathValue;

        if (node == nullptr)
            return XPathValue::fromString(std::string());
        if (property == "width")
            return XPathValue::fromNumber(node->layout.frame.width);
        if (property == "height")
            return XPathValue::fromNumber(node->layout.frame.height);
        if (property == "text")
            return XPathValue::fromString(node->content.text);
        if (property == "checked")
            return XPathValue::fromBoolean(node->content.checked);
        if (property == "visible")
            return XPathValue::fromBoolean(node->visible);
        if (property == "enabled")
            return XPathValue::fromBoolean(node->enabled);
        if (property == "value")
            return valueProperty(*node);
        return XPathValue::fromString(std::string());
    }

    // Неизвестная переменная — пустая строка (как атрибут, которого нет) / An unknown variable is an empty string (as an absent attribute)
    beresta::xpath::XPathValue variableValue(const data::Variable* variable)
    {
        using beresta::xpath::XPathValue;

        if (variable == nullptr)
            return XPathValue::fromString(std::string());
        if (variable->type == data::VariableType::BOOLEAN)
            return XPathValue::fromBoolean(variable->value == "true");
        if (variable->type == data::VariableType::NUMBER)
            return XPathValue::fromNumber(beresta::xpath::stringToNumber(variable->value));
        return XPathValue::fromString(variable->value);
    }

    /*
    ** Имя модели выделения важнее id узла: $orderSel.current — номер текущей записи (-1 — нет),
    ** $orderSel.count — число отмеченных. Иначе — свойство узла, как раньше.
    **
    ** A selection model name wins over a node id: $orderSel.current is the current record index (-1 — none),
    ** $orderSel.count is the marked count. Otherwise — a node property, as before.
    */
    beresta::xpath::XPathValue watchValue(EvaluationScope& scope, const ExpressionBinding& binding, const PropertyWatch& watch)
    {
        using beresta::xpath::XPathValue;
        const data::SelectionModel* selection = scope.owner.selection(watch.base);

        if (selection == nullptr)
            return propertyValue(propertyNode(scope, binding, watch.base), watch.property);
        if (watch.property == "count")
            return XPathValue::fromNumber(static_cast<double>(selection->count()));
        if (watch.property != "current")
            return XPathValue::fromString(std::string());
        if (selection->current() == data::NO_ROW)
            return XPathValue::fromNumber(-1.0);
        return XPathValue::fromNumber(static_cast<double>(selection->current()));
    }
}
