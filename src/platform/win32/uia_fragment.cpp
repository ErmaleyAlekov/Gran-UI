#include "gran/platform/win32/internal/uia_element.hpp"

#include <cstdint>

namespace gran::platform::win32
{
    /*
    ** Обход дерева: направления UI Automation → a11y::navigate. Родителя корня UI Automation
    ** берёт у провайдера HWND, поэтому для корня он пуст.
    **
    ** The tree walk: the UI Automation directions → a11y::navigate. UI Automation takes the root parent
    ** from the HWND provider, so it is empty for the root.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Navigate(NavigateDirection direction, IRawElementProviderFragment** result)
    {
        static const a11y::Direction DIRECTIONS[] = {a11y::Direction::PARENT, a11y::Direction::NEXT,
            a11y::Direction::PREVIOUS, a11y::Direction::FIRST_CHILD, a11y::Direction::LAST_CHILD};

        if (result == nullptr)
            return E_POINTER;
        *result = nullptr;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        if (direction < NavigateDirection_Parent || direction > NavigateDirection_LastChild)
            return E_INVALIDARG;
        const ui::Node* target = a11y::navigate(*_host->tree, *_node, DIRECTIONS[direction]);
        *result = elementFor(_host, target);
        return S_OK;
    }

    // UiaAppendRuntimeId + адрес узла (две половины): уникален, пока узел жив / UiaAppendRuntimeId + the node address (two halves): unique while the node lives
    HRESULT STDMETHODCALLTYPE UiaElement::GetRuntimeId(SAFEARRAY** id)
    {
        std::uint64_t address = reinterpret_cast<std::uintptr_t>(_node);
        int parts[3] = {UiaAppendRuntimeId, static_cast<int>(address & 0xFFFFFFFFu), static_cast<int>(address >> 32)};

        if (id == nullptr)
            return E_POINTER;
        *id = SafeArrayCreateVector(VT_I4, 0, 3);
        if (*id == nullptr)
            return E_OUTOFMEMORY;
        for (LONG index = 0; index < 3; ++index)
            SafeArrayPutElement(*id, &index, &parts[index]);
        return S_OK;
    }

    // Рамка узла на экране / The node frame on the screen
    HRESULT STDMETHODCALLTYPE UiaElement::get_BoundingRectangle(UiaRect* rect)
    {
        if (rect == nullptr)
            return E_POINTER;
        *rect = UiaRect{0.0, 0.0, 0.0, 0.0};
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        *rect = screenRect(*_host, *_node);
        return S_OK;
    }

    // Вложенных корней (чужих окон внутри) нет / There are no embedded roots (foreign windows inside)
    HRESULT STDMETHODCALLTYPE UiaElement::GetEmbeddedFragmentRoots(SAFEARRAY** roots)
    {
        if (roots == nullptr)
            return E_POINTER;
        *roots = nullptr;
        return S_OK;
    }

    /*
    ** Перенос фокуса по просьбе клиента (диктор ведёт фокус за своим курсором): запрос в очередь,
    ** маршрутизатор ставит фокус как Tab. Узел, не принимающий фокус, — UIA_E_INVALIDOPERATION.
    **
    ** Moving the focus at a client request (a screen reader moves the focus after its cursor): a request into the queue,
    ** the router puts the focus as Tab does. A node not taking the focus — UIA_E_INVALIDOPERATION.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::SetFocus()
    {
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        if (!a11y::isFocusable(*_node))
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return post(a11y::AccessRequest{a11y::Request::FOCUS, _node, std::string(), 0.0f});
    }
}
