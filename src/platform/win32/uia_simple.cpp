#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Провайдер в процессе приложения по правилам COM: элементы живут в STA потока окна, и UI Automation
    ** вызывает их там же, через очередь сообщений (pollEvent). Без UseComThreading вызовы приходят
    ** из потоков RPC — проверено: дерево читалось бы параллельно с его изменением.
    **
    ** A provider inside the application process by the COM rules: the elements live in the window thread STA, and UI Automation
    ** calls them there, through the message queue (pollEvent). Without UseComThreading the calls come
    ** from RPC threads — verified: the tree would be read in parallel with its changes.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::get_ProviderOptions(ProviderOptions* options)
    {
        if (options == nullptr)
            return E_POINTER;
        *options = static_cast<ProviderOptions>(ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading);
        return S_OK;
    }

    /*
    ** Шаблоны управления (Invoke, Toggle, SelectionItem, Value, RangeValue) реализует сам элемент;
    ** неподдержанный шаблон — nullptr и S_OK (так UI Automation узнаёт, что шаблона нет).
    **
    ** The control patterns (Invoke, Toggle, SelectionItem, Value, RangeValue) are implemented by the element itself;
    ** an unsupported pattern — nullptr and S_OK (this is how UI Automation learns there is no pattern).
    */
    HRESULT STDMETHODCALLTYPE UiaElement::GetPatternProvider(PATTERNID pattern, IUnknown** provider)
    {
        if (provider == nullptr)
            return E_POINTER;
        *provider = patternFor(pattern);
        if (*provider != nullptr)
            (*provider)->AddRef();
        return S_OK;
    }

    /*
    ** Неподдержанное свойство — VT_EMPTY и S_OK (UI Automation возьмёт значение по умолчанию). describe и перевод
    ** в UTF-16 собирают строки и могут бросить: исключение не уходит в UI Automation (ревью Т-75), уже выделенная
    ** строка BSTR освобождается.
    **
    ** An unsupported property — VT_EMPTY and S_OK (UI Automation takes the default value). describe and the UTF-16
    ** conversion build strings and may throw: the exception does not leave into UI Automation (review T-75), an already
    ** allocated BSTR string is freed.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::GetPropertyValue(PROPERTYID property, VARIANT* value)
    {
        if (value == nullptr)
            return E_POINTER;
        VariantInit(value);
        Presence state = presence();
        if (state == Presence::VIRTUAL)
            virtualProperty(property, value);
        if (state == Presence::VIRTUAL)
            return S_OK;
        if (state == Presence::GONE)
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        try
        {
            nodeProperty(*_host, *_node, property, value);
        }
        catch (...)
        {
            VariantClear(value);
            return E_OUTOFMEMORY;
        }
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
