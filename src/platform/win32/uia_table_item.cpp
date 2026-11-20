#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::platform::win32
{
    // У ячейки: заголовков строк нет (как у таблицы) / For a cell: there are no row headers (as for the table)
    HRESULT STDMETHODCALLTYPE UiaElement::GetRowHeaderItems(SAFEARRAY** headers)
    {
        HRESULT status = check(a11y::PATTERN_GRID_ITEM);

        if (headers == nullptr)
            return E_POINTER;
        *headers = nullptr;
        if (FAILED(status))
            return status;
        if (!elementArray(_host, std::vector<const ui::Node*>(), headers))
            return E_OUTOFMEMORY;
        return S_OK;
    }

    // Заголовок своего столбца: диктор читает его перед значением ячейки / The own column header: the screen reader reads it before the cell value
    HRESULT STDMETHODCALLTYPE UiaElement::GetColumnHeaderItems(SAFEARRAY** headers)
    {
        HRESULT status = check(a11y::PATTERN_GRID_ITEM);

        if (headers == nullptr)
            return E_POINTER;
        *headers = nullptr;
        if (FAILED(status))
            return status;
        // Шаблон GridItem у ячейки есть, только когда она в строке таблицы; проверка — на случай смены этого правила / The GridItem pattern exists at a cell only inside a table row; the check is in case that rule changes
        const ui::Node* grid = a11y::collectionOf(*_node);
        if (grid == nullptr || _node->parent == nullptr)
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        try
        {
            std::size_t column = ui::cellColumn(*_node->parent, *_node);
            if (!elementArray(_host, columnHeaders(*grid, column), headers))
                return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
}
