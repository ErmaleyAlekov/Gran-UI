#include "gran/platform/win32/internal/uia_element.hpp"

#include <climits>

namespace gran::platform::win32
{
    /*
    ** SAFEARRAY из VT_UNKNOWN: каждый элемент кладётся со своей ссылкой, массив забирает клиент. Создать элемент
    ** не вышло — массив уничтожается целиком (SafeArrayDestroy освобождает уже положенные ссылки).
    **
    ** A SAFEARRAY of VT_UNKNOWN: every element is put with its own reference, the client takes the array. Creating an element
    ** failed — the whole array is destroyed (SafeArrayDestroy releases the references already put).
    */
    bool elementArray(const std::shared_ptr<UiaHost>& host, const std::vector<const ui::Node*>& nodes, SAFEARRAY** result) noexcept
    {
        *result = nullptr;
        // Индекс SAFEARRAY — LONG: больше узлов в массив не положить (ревью Т-75) / A SAFEARRAY index is a LONG: more nodes do not fit into the array (review T-75)
        if (nodes.size() > static_cast<std::size_t>(LONG_MAX))
            return false;
        *result = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(nodes.size()));
        if (*result == nullptr)
            return false;
        for (LONG index = 0; index < static_cast<LONG>(nodes.size()); ++index)
        {
            UiaElement* element = elementFor(host, nodes[static_cast<std::size_t>(index)]);
            HRESULT status = E_OUTOFMEMORY;
            if (element != nullptr)
                status = SafeArrayPutElement(*result, &index, static_cast<IRawElementProviderSimple*>(element));
            if (element != nullptr)
                element->Release();
            if (FAILED(status))
            {
                SafeArrayDestroy(*result);
                *result = nullptr;
                return false;
            }
        }
        return true;
    }

    // Выделенные строки среди построенных копий (у виртуального списка — около видимой области) / The selected rows among the built copies (for a virtual list — near the visible area)
    HRESULT STDMETHODCALLTYPE UiaElement::GetSelection(SAFEARRAY** selection)
    {
        HRESULT status = check(a11y::PATTERN_SELECTION);

        if (selection == nullptr)
            return E_POINTER;
        *selection = nullptr;
        if (FAILED(status))
            return status;
        try
        {
            if (!elementArray(_host, a11y::selectedItems(*_node), selection))
                return E_OUTOFMEMORY;
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }

    // Режим выделения модели: MULTI и EXTENDED — несколько строк / The model selection mode: MULTI and EXTENDED — several rows
    HRESULT STDMETHODCALLTYPE UiaElement::get_CanSelectMultiple(BOOL* multiple)
    {
        HRESULT status = check(a11y::PATTERN_SELECTION);

        if (multiple == nullptr)
            return E_POINTER;
        *multiple = FALSE;
        if (FAILED(status))
            return status;
        // Шаблон Selection есть только у таблицы и дерева со списком строк; проверка — на случай смены этого правила / The Selection pattern exists only at a table and a tree with a row list; the check is in case that rule changes
        const ui::Node* list = a11y::itemListOf(*_node);
        if (list != nullptr && _host->tree->items != nullptr && _host->tree->items->multipleSelection(*list))
            *multiple = TRUE;
        return S_OK;
    }

    // Выделение можно снять целиком (Ctrl+щелчок, код программы) — оно не обязательно / The selection can be dropped entirely (Ctrl+click, the program code) — it is not required
    HRESULT STDMETHODCALLTYPE UiaElement::get_IsSelectionRequired(BOOL* required)
    {
        HRESULT status = check(a11y::PATTERN_SELECTION);

        if (required == nullptr)
            return E_POINTER;
        *required = FALSE;
        return status;
    }

    /*
    ** У строки таблицы и дерева контейнер выделения — сама таблица или дерево. У радиокнопок такого узла нет
    ** (они группируются по имени, а не по узлу), поэтому ответ пустой — UI Automation это допускает.
    **
    ** For a table and tree row the selection container is the table or the tree itself. Radio buttons have no such node
    ** (they are grouped by name, not by a node), so the answer is empty — UI Automation allows it.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::get_SelectionContainer(IRawElementProviderSimple** container)
    {
        if (container == nullptr)
            return E_POINTER;
        *container = nullptr;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        a11y::Role role = a11y::roleOf(*_node);
        const ui::Node* collection = a11y::collectionOf(*_node);
        if ((role != a11y::Role::DATA_ITEM && role != a11y::Role::TREE_ITEM) || collection == nullptr)
            return S_OK;
        *container = elementFor(_host, collection);
        if (*container == nullptr)
            return E_OUTOFMEMORY;
        return S_OK;
    }
}
