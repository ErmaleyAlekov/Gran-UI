#include "gran/platform/win32/internal/drop_target.hpp"

namespace gran::platform::win32
{
    // Приёмник создаётся с одной ссылкой — её держит окно / The receiver is created with one reference — the window holds it
    DropTarget::DropTarget(Win32Window& window) noexcept
        : _window(&window)
    {
    }

    // Интерфейсы: IUnknown и IDropTarget / The interfaces: IUnknown and IDropTarget
    HRESULT STDMETHODCALLTYPE DropTarget::QueryInterface(REFIID id, void** object) noexcept
    {
        if (object == nullptr)
            return E_POINTER;
        *object = nullptr;
        if (!IsEqualIID(id, IID_IUnknown) && !IsEqualIID(id, IID_IDropTarget))
            return E_NOINTERFACE;
        *object = static_cast<IDropTarget*>(this);
        AddRef();
        return S_OK;
    }

    // Счётчик атомарный: ОС может держать ссылку и отпускать её из своего кода / The counter is atomic: the OS may hold a reference and release it from its own code
    ULONG STDMETHODCALLTYPE DropTarget::AddRef() noexcept
    {
        return ++_references;
    }

    // Последняя ссылка удаляет объект / The last reference deletes the object
    ULONG STDMETHODCALLTYPE DropTarget::Release() noexcept
    {
        ULONG left = --_references;

        if (left == 0)
            delete this;
        return left;
    }

    // После отвязки окно больше не трогается / After detaching the window is not touched anymore
    void DropTarget::detach() noexcept
    {
        _window = nullptr;
        _files = false;
    }
}
