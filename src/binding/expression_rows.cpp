#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    /*
    ** Переменные строки задаются у каждой копии шаблона; вне плоского дерева $depth = 0 и признаки ложны,
    ** без выделения $selected и $current ложны — шаблон работает с любым списком.
    **
    ** The row variables are set for every template copy; outside a flat tree $depth = 0 and the flags are false,
    ** without a selection $selected and $current are false — a template works with any list.
    */
    void rowVariables(const ItemRef& item, beresta::xpath::XPathContext& context)
    {
        using beresta::xpath::XPathValue;
        const data::TreeList* rows = dynamic_cast<const data::TreeList*>(item.model);
        bool selected = item.selection != nullptr && item.selection->isSelected(item.index);
        bool current = item.selection != nullptr && item.selection->current() == item.index;
        double depth = 0.0;
        bool expanded = false;
        bool children = false;

        if (rows != nullptr && item.index < rows->size())
        {
            depth = static_cast<double>(rows->depth(item.index));
            expanded = rows->expanded(item.index);
            children = rows->hasChildren(item.index);
        }
        context.variables.insert_or_assign("selected", XPathValue::fromBoolean(selected));
        context.variables.insert_or_assign("current", XPathValue::fromBoolean(current));
        context.variables.insert_or_assign("depth", XPathValue::fromNumber(depth));
        context.variables.insert_or_assign("expanded", XPathValue::fromBoolean(expanded));
        context.variables.insert_or_assign("hasChildren", XPathValue::fromBoolean(children));
    }

    bool rowVariable(std::string_view name) noexcept
    {
        return name == "selected" || name == "current" || name == "depth" || name == "expanded"
            || name == "hasChildren";
    }
}
