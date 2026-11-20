#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/ui/data_grid.hpp"

namespace gran::platform::win32
{
    // Ячейки первой строки GRID_HEADER (вторая — строка фильтров) / The cells of the first GRID_HEADER line (the second is the filter row)
    std::vector<const ui::Node*> columnHeaders(const ui::Node& grid, std::size_t column)
    {
        std::vector<const ui::Node*> result;
        const ui::Node* header = ui::gridHeader(const_cast<ui::Node&>(grid));

        if (header == nullptr)
            return result;
        for (std::size_t index = 0; index < header->children.size(); ++index)
        {
            const ui::Node& cell = *header->children[index];
            if (cell.visible && (column == ui::NO_COLUMN || column == index))
                result.push_back(&cell);
        }
        return result;
    }

    // Заголовков строк у таблицы нет — пустой массив, а не ошибка / The table has no row headers — an empty array, not an error
    HRESULT STDMETHODCALLTYPE UiaElement::GetRowHeaders(SAFEARRAY** headers)
    {
        HRESULT status = check(a11y::PATTERN_GRID);

        if (headers == nullptr)
            return E_POINTER;
        *headers = nullptr;
        if (FAILED(status))
            return status;
        if (!elementArray(_host, std::vector<const ui::Node*>(), headers))
            return E_OUTOFMEMORY;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE UiaElement::GetColumnHeaders(SAFEARRAY** headers)
    {
        HRESULT status = check(a11y::PATTERN_GRID);

        if (headers == nullptr)
            return E_POINTER;
        *headers = nullptr;
        if (FAILED(status))
            return status;
        try
        {
            if (!elementArray(_host, columnHeaders(*_node, ui::NO_COLUMN), headers))
                return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    // Строки идут первыми: таблица — список записей, столбцы — их поля / The rows come first: a table is a list of records, the columns are their fields
    HRESULT STDMETHODCALLTYPE UiaElement::get_RowOrColumnMajor(enum RowOrColumnMajor* major)
    {
        HRESULT status = check(a11y::PATTERN_GRID);

        if (major == nullptr)
            return E_POINTER;
        *major = RowOrColumnMajor_RowMajor;
        return status;
    }
}
