#include "gran/a11y/access_tree.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::a11y
{
    /*
    ** Ближайший предок-таблица или дерево. Вложенный список в шаблоне строки тоже окажется внутри таблицы, но его строки —
    ** обычные узлы (не GRID_ROW и не TREE_ROW), поэтому чужие роли им не достаются.
    **
    ** The nearest table or tree ancestor. A nested list in the row template is inside the table too, but its rows are
    ** ordinary nodes (neither GRID_ROW nor TREE_ROW), so they do not get alien roles.
    */
    const ui::Node* collectionOf(const ui::Node& node) noexcept
    {
        for (const ui::Node* current = node.parent; current != nullptr; current = current->parent)
        {
            if (current->kind == ui::ElementKind::DATA_GRID || current->kind == ui::ElementKind::TREE_VIEW)
                return current;
        }
        return nullptr;
    }

    // Части строит движок; функции ui не меняют узел, а лишь отдают его ребёнка / The engine builds the parts; the ui functions do not change the node, they only give its child
    const ui::Node* itemListOf(const ui::Node& collection) noexcept
    {
        ui::Node& node = const_cast<ui::Node&>(collection);

        if (collection.kind == ui::ElementKind::DATA_GRID)
            return ui::gridRows(node);
        if (collection.kind == ui::ElementKind::TREE_VIEW)
            return ui::treeViewRows(node);
        return nullptr;
    }

    /*
    ** Выделять можно, если у списка строк есть модель выделения. Сеткой (Grid, GridItem) считаются только строки
    ** основного списка таблицы: у закреплённых сверху и снизу строк свои модели и свои номера записей.
    ** Раскрываются строка с кнопкой раскрытия и ячейка столбца-дерева, где кнопка стоит.
    **
    ** Selecting is possible if the row list has a selection model. Only the rows of the main table list count as the grid
    ** (Grid, GridItem): the rows pinned at the top and the bottom have their own models and their own record indexes.
    ** A row with an expander button and the tree column cell where the button stands expand.
    */
    std::uint16_t collectionPatterns(const ui::Node& node) noexcept
    {
        Role role = roleOf(node);
        bool whole = role == Role::DATA_GRID || role == Role::TREE;
        const ui::Node* collection = collectionOf(node);
        const ui::Node* list = nullptr;
        unsigned patterns = 0;

        if (whole)
            collection = &node;
        if (collection != nullptr)
            list = itemListOf(*collection);
        if (whole && list != nullptr && !list->content.selection.empty())
            patterns |= PATTERN_SELECTION;
        if (role == Role::DATA_GRID && list != nullptr)
            patterns |= PATTERN_GRID;
        if ((role == Role::DATA_ITEM || role == Role::TREE_ITEM) && node.parent != nullptr
            && !node.parent->content.selection.empty())
            patterns |= PATTERN_SELECT;
        if (role == Role::CELL && list != nullptr && node.parent != nullptr && node.parent->parent == list)
            patterns |= PATTERN_GRID_ITEM;
        if (itemExpander(node) != nullptr)
            patterns |= PATTERN_EXPAND;
        return static_cast<std::uint16_t>(patterns);
    }

    // Только видимые копии: распорки виртуального списка скрыты / Only the visible copies: the virtual list spacers are hidden
    std::vector<const ui::Node*> selectedItems(const ui::Node& collection)
    {
        std::vector<const ui::Node*> result;
        const ui::Node* list = itemListOf(collection);

        if (list == nullptr)
            return result;
        for (const std::unique_ptr<ui::Node>& child : list->children)
        {
            if (child->visible && isSelected(*child))
                result.push_back(child.get());
        }
        return result;
    }

    // Тот же отбор, что у selectedItems / The same choice as in selectedItems
    bool anySelected(const ui::Node& collection) noexcept
    {
        const ui::Node* list = itemListOf(collection);

        if (list == nullptr)
            return false;
        for (const std::unique_ptr<ui::Node>& child : list->children)
        {
            if (child->visible && isSelected(*child))
                return true;
        }
        return false;
    }
}
