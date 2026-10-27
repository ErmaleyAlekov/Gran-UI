#include "gran/ui/node.hpp"

namespace gran::ui
{
    // Подменю построитель кладёт единственным ребёнком пункта (так требует схема) / The builder puts a submenu as the only item child (the schema requires so)
    Node* submenuOf(const Node& item) noexcept
    {
        if (item.kind != ElementKind::MENU_ITEM || item.children.empty())
            return nullptr;
        Node* last = item.children.back().get();
        if (last->kind != ElementKind::POPUP)
            return nullptr;
        return last;
    }

    // Скрытый предок прячет всё поддерево (закрытый слой — тоже) / A hidden ancestor hides the whole subtree (a closed layer too)
    bool isShown(const Node& node) noexcept
    {
        for (const Node* current = &node; current != nullptr; current = current->parent)
        {
            if (!current->visible)
                return false;
        }
        return true;
    }

    // Значение из разметки сохраняется, даже если такого пункта (пока) нет: подпись тогда пуста
    // The value from the markup is kept even if there is no such item (yet): the label is empty then
    void syncComboBoxes(Node& subtree)
    {
        if (subtree.kind == ElementKind::COMBO_BOX)
        {
            std::string value = subtree.content.option;
            selectOption(subtree, findOption(subtree, value));
            subtree.content.option = value;
        }
        for (const std::unique_ptr<Node>& child : subtree.children)
            syncComboBoxes(*child);
    }
}
