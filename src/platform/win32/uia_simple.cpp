#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    // Провайдер в процессе приложения; COM-потоки не нужны — вызовы приходят в поток окна
    // A provider inside the application process; COM threading is not needed — the calls come on the window thread
    HRESULT STDMETHODCALLTYPE UiaElement::get_ProviderOptions(ProviderOptions* options)
    {
        if (options == nullptr)
            return E_POINTER;
        *options = ProviderOptions_ServerSideProvider;
        return S_OK;
    }

    /*
    ** Шаблоны управления (Invoke, Toggle, Value, RangeValue) — следующий шаг: сведения для них
    ** уже есть в a11y::AccessibleInfo. Пока клиент получает только имена, роли и состояние.
    **
    ** The control patterns (Invoke, Toggle, Value, RangeValue) are the next step: the facts for them
    ** are already in a11y::AccessibleInfo. For now a client gets only names, roles and state.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::GetPatternProvider(PATTERNID pattern, IUnknown** provider)
    {
        (void)pattern;
        if (provider == nullptr)
            return E_POINTER;
        *provider = nullptr;
        return S_OK;
    }

    // Неподдержанное свойство — VT_EMPTY и S_OK (UI Automation возьмёт значение по умолчанию)
    // An unsupported property — VT_EMPTY and S_OK (UI Automation takes the default value)
    HRESULT STDMETHODCALLTYPE UiaElement::GetPropertyValue(PROPERTYID property, VARIANT* value)
    {
        if (value == nullptr)
            return E_POINTER;
        value->vt = VT_EMPTY;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        nodeProperty(*_host, *_node, property, value);
        return S_OK;
    }

    // Корень опирается на провайдер HWND (заголовок окна, положение); остальные — нет
    // The root relies on the HWND provider (the window title, the position); the others do not
    HRESULT STDMETHODCALLTYPE UiaElement::get_HostRawElementProvider(IRawElementProviderSimple** provider)
    {
        if (provider == nullptr)
            return E_POINTER;
        *provider = nullptr;
        if (_host->tree == nullptr || _node != _host->tree->root)
            return S_OK;
        return UiaHostProviderFromHwnd(_host->window, provider);
    }
}
