#include "gran/ui/node.hpp"

namespace gran::ui
{
    // Список — последний ребёнок вида Popup (его добавляет построитель разметки) / The list is the last child of the Popup kind (the markup builder adds it)
    Node* comboList(Node& combo) noexcept
    {
        if (combo.kind != ElementKind::COMBO_BOX || combo.children.empty())
            return nullptr;
        Node& last = *combo.children.back();
        if (last.kind != ElementKind::POPUP)
            return nullptr;
        return &last;
    }

    // Как у <option> в HTML: без value значением служит подпись / As for an HTML <option>: without value the label serves as the value
    const std::string& optionValue(const Node& item) noexcept
    {
        if (item.content.option.empty())
            return item.content.text;
        return item.content.option;
    }

    // Пункты — прямые дети списка (копии шаблона тоже встают туда) / The items are the direct list children (the template copies go there too)
    Node* findOption(Node& combo, const std::string& value) noexcept
    {
        Node* list = comboList(combo);

        if (list == nullptr)
            return nullptr;
        for (const std::unique_ptr<Node>& child : list->children)
        {
            if (child->kind == ElementKind::MENU_ITEM && optionValue(*child) == value)
                return child.get();
        }
        return nullptr;
    }

    /*
    ** Выбор переносит в ComboBox значение и подпись пункта (подпись меняет размер текста —
    ** нужна компоновка), отметка Checked остаётся только у выбранного пункта.
    **
    ** The choice carries the item value and label into the ComboBox (the label changes the text size —
    ** a layout is needed), the Checked mark stays only on the selected item.
    */
    bool selectOption(Node& combo, Node* item)
    {
        Node* list = comboList(combo);
        std::string value;
        std::string label;

        if (list == nullptr)
            return false;
        if (item != nullptr)
        {
            value = optionValue(*item);
            label = item->content.text;
        }
        for (const std::unique_ptr<Node>& child : list->children)
            setChecked(*child, child.get() == item);
        bool changed = combo.content.option != value;
        combo.content.option = value;
        if (combo.content.text != label)
        {
            combo.content.text = label;
            markDirty(combo, DIRTY_LAYOUT | DIRTY_PAINT);
        }
        return changed;
    }
}
