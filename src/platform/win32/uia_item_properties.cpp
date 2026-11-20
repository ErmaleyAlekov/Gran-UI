#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/ui/data_grid.hpp"

#include <climits>

namespace gran::platform::win32
{
    namespace
    {
        // Целое свойство (VT_I4) с пределом int / An integer property (VT_I4) limited to int
        bool setNumber(VARIANT* value, std::size_t number) noexcept
        {
            if (number > static_cast<std::size_t>(INT_MAX))
                number = static_cast<std::size_t>(INT_MAX);
            value->vt = VT_I4;
            value->lVal = static_cast<LONG>(number);
            return true;
        }

        /*
        ** Сортировка у заголовка столбца — как aria-sort в браузерах (свойство AriaProperties «sort=ascending»): диктор
        ** говорит «по возрастанию», а стрелка «▲» в имени не звучит. Несортированный столбец — без свойства.
        **
        ** The sorting at a column header — as aria-sort in browsers (the AriaProperties "sort=ascending" property): the screen
        ** reader says "ascending", and the "▲" arrow is not in the name. An unsorted column has no property.
        */
        bool sortProperty(const ui::Node& header, VARIANT* value) noexcept
        {
            const ui::Node* grid = a11y::collectionOf(header);

            if (grid == nullptr || !grid->content.grid || header.parent == nullptr)
                return false;
            const ui::GridState& state = *grid->content.grid;
            if (state.sorted == ui::NO_COLUMN || state.sorted >= header.parent->children.size()
                || header.parent->children[state.sorted].get() != &header)
                return false;
            const wchar_t* sort = L"sort=ascending";
            if (state.descending)
                sort = L"sort=descending";
            value->vt = VT_BSTR;
            value->bstrVal = SysAllocString(sort);
            return value->bstrVal != nullptr;
        }
    }

    /*
    ** Уровень строки дерева (с 1, как aria-level) — диктор говорит «уровень 2»; у строки таблицы — место среди
    ** всех записей и их число («строка 5 из 1000»), хотя копий у виртуального списка лишь часть.
    **
    ** The tree row level (from 1, as aria-level) — the screen reader says "level 2"; for a table row — the place among
    ** all the records and their count ("row 5 of 1000"), although a virtual list has copies only for a part.
    */
    bool itemProperty(const UiaHost& host, const ui::Node& node, PROPERTYID property, VARIANT* value) noexcept
    {
        a11y::Role role = a11y::roleOf(node);
        const a11y::ItemSource* items = host.tree->items;

        if (role == a11y::Role::HEADER_ITEM && property == UIA_AriaPropertiesPropertyId)
            return sortProperty(node, value);
        if (items == nullptr || (role != a11y::Role::TREE_ITEM && role != a11y::Role::DATA_ITEM))
            return false;
        if (property == UIA_LevelPropertyId && a11y::itemExpander(node) != nullptr)
            return setNumber(value, items->itemDepth(node) + 1);
        if (role != a11y::Role::DATA_ITEM || node.parent == nullptr)
            return false;
        std::size_t index = items->itemIndex(node);
        if (index == a11y::NO_ITEM)
            return false;
        if (property == UIA_PositionInSetPropertyId)
            return setNumber(value, index + 1);
        if (property == UIA_SizeOfSetPropertyId)
            return setNumber(value, items->itemCount(*node.parent));
        return false;
    }
}
