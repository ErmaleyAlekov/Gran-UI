#include "gran/platform/win32/internal/uia_element.hpp"

#include <new>

namespace gran::platform::win32
{
    // Корень фрагментов — элемент корня дерева / The fragment root is the tree root element
    HRESULT STDMETHODCALLTYPE UiaElement::get_FragmentRoot(IRawElementProviderFragmentRoot** root)
    {
        if (root == nullptr)
            return E_POINTER;
        *root = nullptr;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        *root = elementFor(_host, _host->tree->root);
        return S_OK;
    }

    // Пиксели экрана → логические координаты окна → попадание как у мыши / Screen pixels → logical window coordinates → a hit as the mouse has
    HRESULT STDMETHODCALLTYPE UiaElement::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result)
    {
        POINT point{static_cast<LONG>(x), static_cast<LONG>(y)};
        float scale = static_cast<float>(GetDpiForWindow(_host->window)) / 96.0f;

        if (result == nullptr)
            return E_POINTER;
        *result = nullptr;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        ScreenToClient(_host->window, &point);
        *result = elementFor(_host, a11y::nodeAt(*_host->tree, Vec2{point.x / scale, point.y / scale}));
        return S_OK;
    }

    // Узел в фокусе по данным приложения / The focused node by the application data
    HRESULT STDMETHODCALLTYPE UiaElement::GetFocus(IRawElementProviderFragment** result)
    {
        if (result == nullptr)
            return E_POINTER;
        *result = nullptr;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        *result = elementFor(_host, _host->tree->focus);
        return S_OK;
    }

    // nothrow: исключение не должно уйти в UI Automation; нехватка памяти — «элемента нет»
    // nothrow: an exception must not leave into UI Automation; out of memory — "no element"
    UiaElement* elementFor(const std::shared_ptr<UiaHost>& host, const ui::Node* node) noexcept
    {
        if (node == nullptr || host->tree == nullptr)
            return nullptr;
        return new (std::nothrow) UiaElement(host, node);
    }
}
