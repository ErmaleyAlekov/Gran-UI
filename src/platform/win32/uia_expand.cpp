#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Раскрыть подменю пункта (диктор, автотест): то же, что стрелка вправо или наведение, но через очередь
    ** запросов — дерево меняет цикл приложения, а не вызов UI Automation.
    **
    ** Expanding the item submenu (a screen reader, a UI test): the same as the right arrow or hovering, but through the
    ** request queue — the application loop changes the tree, not the UI Automation call.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Expand()
    {
        HRESULT status = check(a11y::PATTERN_EXPAND);

        if (FAILED(status))
            return status;
        return post(a11y::AccessRequest{a11y::Request::EXPAND, _node, std::string(), 0.0f});
    }

    // Свернуть подменю (как Esc или стрелка влево в нём) / Collapsing the submenu (as Esc or the left arrow in it)
    HRESULT STDMETHODCALLTYPE UiaElement::Collapse()
    {
        HRESULT status = check(a11y::PATTERN_EXPAND);

        if (FAILED(status))
            return status;
        return post(a11y::AccessRequest{a11y::Request::COLLAPSE, _node, std::string(), 0.0f});
    }

    // Подменю открыто — «раскрыт», иначе «свёрнут» / The submenu is open — "expanded", otherwise "collapsed"
    HRESULT STDMETHODCALLTYPE UiaElement::get_ExpandCollapseState(ExpandCollapseState* state)
    {
        HRESULT status = check(a11y::PATTERN_EXPAND);

        if (state == nullptr)
            return E_POINTER;
        *state = ExpandCollapseState_Collapsed;
        if (FAILED(status))
            return status;
        const ui::Node* submenu = ui::submenuOf(*_node);
        if (submenu != nullptr && submenu->visible)
            *state = ExpandCollapseState_Expanded;
        return S_OK;
    }
}
