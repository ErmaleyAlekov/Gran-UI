#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Выбор радиокнопки или вкладки (как щелчок): соседи по группе снимаются, onChange. Уже выбранный
    ** узел запроса не ставит — выбор не меняется.
    **
    ** Selecting a radio button or a tab (as a click): the group neighbors are cleared, onChange. An already selected
    ** node queues no request — the choice does not change.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Select()
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (FAILED(status))
            return status;
        if (_node->content.checked)
            return S_OK;
        return post(a11y::AccessRequest{a11y::Request::SELECT, _node, std::string(), 0.0f});
    }

    // Группа выбирает ровно один: добавить к выбору можно только уже выбранный / The group selects exactly one: only the already selected one can be added
    HRESULT STDMETHODCALLTYPE UiaElement::AddToSelection()
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (FAILED(status))
            return status;
        if (!_node->content.checked)
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return S_OK;
    }

    // Снять выбор с радиокнопки или вкладки нельзя (только выбрать другую) / A radio button or a tab cannot be deselected (only another one selected)
    HRESULT STDMETHODCALLTYPE UiaElement::RemoveFromSelection()
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (FAILED(status))
            return status;
        if (_node->content.checked)
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return S_OK;
    }

    // Выбран — отмечен (у заголовка вкладки отметка — выбранная страница) / Selected means checked (for a tab header the mark is the selected page)
    HRESULT STDMETHODCALLTYPE UiaElement::get_IsSelected(BOOL* selected)
    {
        HRESULT status = check(a11y::PATTERN_SELECT);

        if (selected == nullptr)
            return E_POINTER;
        *selected = FALSE;
        if (FAILED(status))
            return status;
        if (_node->content.checked)
            *selected = TRUE;
        return S_OK;
    }

    /*
    ** Контейнера с шаблоном Selection у группы нет (радиокнопки группируются по имени, а не по узлу),
    ** поэтому ответ пустой — UI Automation это допускает.
    **
    ** A group has no container with the Selection pattern (radio buttons are grouped by name, not by a node),
    ** so the answer is empty — UI Automation allows it.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::get_SelectionContainer(IRawElementProviderSimple** container)
    {
        if (container == nullptr)
            return E_POINTER;
        *container = nullptr;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        return S_OK;
    }
}
