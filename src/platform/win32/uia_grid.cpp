#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/ui/data_grid.hpp"

#include <climits>

namespace gran::platform::win32
{
    namespace
    {
        // Построенные строки списка (распорки виртуального списка скрыты) / The built list rows (the virtual list spacers are hidden)
        std::size_t visibleRows(const ui::Node& list) noexcept
        {
            std::size_t rows = 0;

            for (const std::unique_ptr<ui::Node>& child : list.children)
            {
                if (child->visible && child->kind == ui::ElementKind::GRID_ROW)
                    ++rows;
            }
            return rows;
        }
    }

    // Элемент — сама таблица (шаблон Grid проверен), её список строк строит движок / The element is the table itself (the Grid pattern is checked), the engine builds its row list
    const ui::Node* UiaElement::rowList() const noexcept
    {
        return a11y::itemListOf(*_node);
    }

    /*
    ** Ячейка по номеру записи и столбцу (порядок столбцов — как на экране). Запись вне окна виртуального списка
    ** копии не имеет: ответ пустой (S_OK и nullptr), а не ошибка, — номер правильный, просто элемент сейчас не построен.
    **
    ** A cell by the record index and the column (the column order is as on the screen). A record outside the virtual list window
    ** has no copy: the answer is empty (S_OK and nullptr), not an error — the index is right, the element is just not built now.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::GetItem(int row, int column, IRawElementProviderSimple** item)
    {
        HRESULT status = check(a11y::PATTERN_GRID);
        int rows = 0;
        int columns = 0;

        if (item == nullptr)
            return E_POINTER;
        *item = nullptr;
        if (FAILED(status) || FAILED(get_RowCount(&rows)) || FAILED(get_ColumnCount(&columns)))
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        if (row < 0 || column < 0 || row >= rows || column >= columns)
            return E_INVALIDARG;
        const ui::Node* line = nullptr;
        if (_host->tree->items != nullptr && rowList() != nullptr)
            line = _host->tree->items->itemAt(*rowList(), static_cast<std::size_t>(row));
        if (line == nullptr || !line->visible)
            return S_OK;
        const ui::Node* cell = ui::gridCell(const_cast<ui::Node&>(*line), static_cast<std::size_t>(column));
        if (cell == nullptr)
            return S_OK;
        *item = elementFor(_host, cell);
        if (*item == nullptr)
            return E_OUTOFMEMORY;
        return S_OK;
    }

    // Записей в источнике строк (без сведений о строках — только построенные копии) / The records in the row source (without the row facts — only the built copies)
    HRESULT STDMETHODCALLTYPE UiaElement::get_RowCount(int* count)
    {
        HRESULT status = check(a11y::PATTERN_GRID);
        std::size_t rows = 0;

        if (count == nullptr)
            return E_POINTER;
        *count = 0;
        if (FAILED(status))
            return status;
        const ui::Node* list = rowList();
        if (_host->tree->items != nullptr)
            rows = _host->tree->items->itemCount(*list);
        else
            rows = visibleRows(*list);
        if (rows > static_cast<std::size_t>(INT_MAX))
            rows = static_cast<std::size_t>(INT_MAX);
        *count = static_cast<int>(rows);
        return S_OK;
    }

    // Столбцов — сколько описано в таблице (виртуализация столбцов ячейки не убирает) / The columns — as many as the table describes (column virtualization does not remove cells)
    HRESULT STDMETHODCALLTYPE UiaElement::get_ColumnCount(int* count)
    {
        HRESULT status = check(a11y::PATTERN_GRID);
        std::size_t columns = 0;

        if (count == nullptr)
            return E_POINTER;
        *count = 0;
        if (FAILED(status))
            return status;
        if (_node->content.grid)
            columns = _node->content.grid->columns.size();
        if (columns > static_cast<std::size_t>(INT_MAX))
            columns = static_cast<std::size_t>(INT_MAX);
        *count = static_cast<int>(columns);
        return S_OK;
    }
}
