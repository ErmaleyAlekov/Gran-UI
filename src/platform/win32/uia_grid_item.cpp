#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/ui/data_grid.hpp"

#include <climits>

namespace gran::platform::win32
{
    /*
    ** Строка ячейки — номер записи её строки в источнике (тот же, что у GetItem таблицы). Сведений о строках
    ** нет или копия уже не в списке — UIA_E_ELEMENTNOTAVAILABLE (шаблон GridItem проверен: строка — копия основного списка).
    **
    ** The cell row is the record index of its row in the source (the same as for the table GetItem). There are no row facts
    ** or the copy is no longer in the list — UIA_E_ELEMENTNOTAVAILABLE (the GridItem pattern is checked: the row is a main list copy).
    */
    HRESULT STDMETHODCALLTYPE UiaElement::get_Row(int* row)
    {
        HRESULT status = check(a11y::PATTERN_GRID_ITEM);
        std::size_t index = a11y::NO_ITEM;

        if (row == nullptr)
            return E_POINTER;
        *row = 0;
        if (FAILED(status))
            return status;
        if (_host->tree->items != nullptr)
            index = _host->tree->items->itemIndex(*_node->parent);
        // NO_ITEM (не копия строки) больше INT_MAX — отсекается той же проверкой / NO_ITEM (not a row copy) is above INT_MAX — the same check cuts it off
        if (index > static_cast<std::size_t>(INT_MAX))
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        *row = static_cast<int>(index);
        return S_OK;
    }

    // Столбец — место ячейки в строке, то есть порядок на экране (перестановка столбцов переносит ячейки) / The column is the cell place in the row, that is the screen order (reordering the columns moves the cells)
    HRESULT STDMETHODCALLTYPE UiaElement::get_Column(int* column)
    {
        HRESULT status = check(a11y::PATTERN_GRID_ITEM);

        if (column == nullptr)
            return E_POINTER;
        *column = 0;
        if (FAILED(status))
            return status;
        std::size_t index = ui::cellColumn(*_node->parent, *_node);
        // NO_COLUMN (ячейка не в строке) больше INT_MAX — отсекается той же проверкой / NO_COLUMN (the cell is not in the row) is above INT_MAX — the same check cuts it off
        if (index > static_cast<std::size_t>(INT_MAX))
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        *column = static_cast<int>(index);
        return S_OK;
    }

    // Объединённых ячеек у таблицы нет / The table has no merged cells
    HRESULT STDMETHODCALLTYPE UiaElement::get_RowSpan(int* span)
    {
        HRESULT status = check(a11y::PATTERN_GRID_ITEM);

        if (span == nullptr)
            return E_POINTER;
        *span = 1;
        return status;
    }

    HRESULT STDMETHODCALLTYPE UiaElement::get_ColumnSpan(int* span)
    {
        HRESULT status = check(a11y::PATTERN_GRID_ITEM);

        if (span == nullptr)
            return E_POINTER;
        *span = 1;
        return status;
    }

    // Таблица, в которой лежит ячейка / The table holding the cell
    HRESULT STDMETHODCALLTYPE UiaElement::get_ContainingGrid(IRawElementProviderSimple** grid)
    {
        HRESULT status = check(a11y::PATTERN_GRID_ITEM);

        if (grid == nullptr)
            return E_POINTER;
        *grid = nullptr;
        if (FAILED(status))
            return status;
        // Таблицы нет — пустой ответ (шаблон GridItem этого не допускает, но правило может смениться); нет элемента — нехватка памяти (ревью Т-75)
        // No table — an empty answer (the GridItem pattern does not allow it, but the rule may change); no element — out of memory (review T-75)
        const ui::Node* table = a11y::collectionOf(*_node);
        if (table == nullptr)
            return S_OK;
        *grid = elementFor(_host, table);
        if (*grid == nullptr)
            return E_OUTOFMEMORY;
        return S_OK;
    }
}
