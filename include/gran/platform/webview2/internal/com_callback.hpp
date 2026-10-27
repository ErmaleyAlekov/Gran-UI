#ifndef GRAN_PLATFORM_WEBVIEW2_INTERNAL_COM_CALLBACK_HPP
#define GRAN_PLATFORM_WEBVIEW2_INTERNAL_COM_CALLBACK_HPP

#include <windows.h>

#include <memory>

namespace gran::platform::webview2
{
    /*
    ** Колбэк COM (обработчик завершения или события WebView2) без лямбд: Invoke вызывает метод
    ** владельца (паттерн Command). Владелец держится слабой ссылкой: браузер может вызвать
    ** колбэк после закрытия встроенного окна — тогда вызов просто пропускается.
    ** Исключение не пересекает границу COM: Invoke возвращает E_FAIL.
    **
    ** A COM callback (a WebView2 completion or event handler) without lambdas: Invoke calls an owner
    ** method (the Command pattern). The owner is held by a weak reference: the browser may call
    ** the callback after the embedded window was closed — then the call is simply skipped.
    ** An exception does not cross the COM boundary: Invoke returns E_FAIL.
    */
    template <typename Interface, typename Owner, typename... Args>
    class ComCallback final : public Interface
    {
    public:
        using Method = HRESULT (Owner::*)(Args...);

        // iid — идентификатор Interface (MinGW не даёт __uuidof для типов WebView2) / iid is the Interface identifier (MinGW gives no __uuidof for the WebView2 types)
        ComCallback(std::weak_ptr<Owner> owner, Method method, const IID& iid) noexcept
            : _owner(std::move(owner)), _method(method), _iid(iid)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) noexcept override
        {
            if (object == nullptr)
                return E_POINTER;
            *object = nullptr;
            if (!IsEqualIID(iid, IID_IUnknown) && !IsEqualIID(iid, _iid))
                return E_NOINTERFACE;
            *object = static_cast<Interface*>(this);
            AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() noexcept override
        {
            return static_cast<ULONG>(InterlockedIncrement(&_count));
        }

        // Последняя ссылка уничтожает объект (создан через new) / The last reference destroys the object (created with new)
        ULONG STDMETHODCALLTYPE Release() noexcept override
        {
            LONG count = InterlockedDecrement(&_count);

            if (count == 0)
                delete this;
            return static_cast<ULONG>(count);
        }

        HRESULT STDMETHODCALLTYPE Invoke(Args... args) noexcept override
        {
            std::shared_ptr<Owner> owner = _owner.lock();

            if (!owner)
                return S_OK;
            try
            {
                return (owner.get()->*_method)(args...);
            }
            catch (...)
            {
                return E_FAIL;
            }
        }

    private:
        std::weak_ptr<Owner> _owner;
        Method _method;
        IID _iid;
        LONG _count = 1;
    };
}

#endif
