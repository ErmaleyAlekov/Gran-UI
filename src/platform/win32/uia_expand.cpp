#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Раскрыть подменю пункта или ветку строки дерева (диктор, автотест): то же, что стрелка вправо или наведение,
    ** но через очередь запросов — дерево меняет цикл приложения, а не вызов UI Automation. Лист раскрыть нельзя.
    **
    ** Expanding the item submenu or the tree row branch (a screen reader, a UI test): the same as the right arrow or hovering,
    ** but through the request queue — the application loop changes the tree, not the UI Automation call. A leaf cannot expand.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Expand()
    {
        HRESULT status = check(a11y::PATTERN_EXPAND);

        if (FAILED(status))
            return status;
        if (a11y::expandStateOf(*_node) == a11y::ExpandState::LEAF)
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return post(a11y::AccessRequest{a11y::Request::EXPAND, _node, std::string(), 0.0f});
    }

    // Свернуть подменю (как Esc или стрелка влево в нём) / Collapsing the submenu (as Esc or the left arrow in it)
    HRESULT STDMETHODCALLTYPE UiaElement::Collapse()
    {
        HRESULT status = check(a11y::PATTERN_EXPAND);

        if (FAILED(status))
            return status;
        if (a11y::expandStateOf(*_node) == a11y::ExpandState::LEAF)
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return post(a11y::AccessRequest{a11y::Request::COLLAPSE, _node, std::string(), 0.0f});
    }

    // Подменю открыто или ветка раскрыта — «раскрыт»; строка дерева без детей — «лист» / The submenu is open or the branch is expanded — "expanded"; a tree row without children — a "leaf"
    HRESULT STDMETHODCALLTYPE UiaElement::get_ExpandCollapseState(ExpandCollapseState* state)
    {
        HRESULT status = check(a11y::PATTERN_EXPAND);

        if (state == nullptr)
            return E_POINTER;
        *state = ExpandCollapseState_Collapsed;
        if (FAILED(status))
            return status;
        *state = expandCollapseOf(a11y::expandStateOf(*_node));
        return S_OK;
    }
}
