#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    namespace
    {
        // Строка таблицы или дерева: выделение ведёт модель выделения списка / A table or tree row: the list selection model keeps the selection
        bool listItem(const ui::Node& node) noexcept
        {
            a11y::Role role = a11y::roleOf(node);

            return role == a11y::Role::DATA_ITEM || role == a11y::Role::TREE_ITEM;
        }
    }

    /*
    ** Выбор радиокнопки или вкладки (как щелчок): соседи по группе снимаются, onChange. Уже выбранный
    ** узел запроса не ставит — выбор не меняется. Строка списка выбирается всегда (как щелчок): выделение
    ** нескольких строк сменится одной этой.
    **
    ** Selecting a radio button or a tab (as a click): the group neighbors are cleared, onChange. An already selected
    ** node queues no request — the choice does not change. A list row is always selected (as a click): a selection
    ** of several rows becomes this one.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Select()
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (FAILED(status))
            return status;
        if (a11y::isSelected(*_node) && !listItem(*_node))
            return S_OK;
        return post(a11y::AccessRequest{a11y::Request::SELECT, _node, std::string(), 0.0f});
    }

    /*
    ** Группа радиокнопок выбирает ровно один: добавить можно только уже выбранный. Строка списка с выделением
    ** нескольких добавляется (как Ctrl+щелчок); при выделении одной — только если ничего не выделено.
    **
    ** A radio group selects exactly one: only the already selected one can be added. A list row with a multiple
    ** selection is added (as Ctrl+click); with a single selection — only if nothing is selected.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::AddToSelection()
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (FAILED(status))
            return status;
        if (a11y::isSelected(*_node))
            return S_OK;
        if (!listItem(*_node))
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        if (_host->tree->items != nullptr && _host->tree->items->multipleSelection(*_node->parent))
            return post(a11y::AccessRequest{a11y::Request::ADD_SELECT, _node, std::string(), 0.0f});
        const ui::Node* collection = a11y::collectionOf(*_node);
        // Без вектора: исключение нехватки памяти не должно уйти в UI Automation (найдено при разборе Т-75) / Without a vector: an out-of-memory exception must not leave into UI Automation (found while handling T-75)
        if (collection != nullptr && a11y::anySelected(*collection))
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return post(a11y::AccessRequest{a11y::Request::SELECT, _node, std::string(), 0.0f});
    }

    // Снять выбор с радиокнопки или вкладки нельзя (только выбрать другую); строку списка — можно (как Ctrl+щелчок) / A radio button or a tab cannot be deselected (only another one selected); a list row can (as Ctrl+click)
    HRESULT STDMETHODCALLTYPE UiaElement::RemoveFromSelection()
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (FAILED(status))
            return status;
        if (!a11y::isSelected(*_node))
            return S_OK;
        if (!listItem(*_node))
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return post(a11y::AccessRequest{a11y::Request::REMOVE_SELECT, _node, std::string(), 0.0f});
    }

    // Выбран — отмечен (у заголовка вкладки отметка — выбранная страница) или выделен моделью выделения / Selected means checked (for a tab header the mark is the selected page) or marked by the selection model
    HRESULT STDMETHODCALLTYPE UiaElement::get_IsSelected(BOOL* selected)
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (selected == nullptr)
            return E_POINTER;
        *selected = FALSE;
        if (FAILED(status))
            return status;
        if (a11y::isSelected(*_node))
            *selected = TRUE;
        return S_OK;
    }
}
