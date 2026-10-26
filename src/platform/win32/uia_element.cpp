#include "gran/platform/win32/internal/uia_element.hpp"

#include <utility>

namespace gran::platform::win32
{
    // Элемент рождается со ссылкой 1 (её получает тот, кто его создал) / The element is born with reference 1 (its creator gets it)
    UiaElement::UiaElement(std::shared_ptr<UiaHost> host, const ui::Node* node)
        : _host(std::move(host)), _node(node)
    {
    }

    // Корень фрагментов отдаётся только элементом корня дерева / The fragment root is given only by the tree root element
    HRESULT STDMETHODCALLTYPE UiaElement::QueryInterface(REFIID riid, void** object)
    {
        if (object == nullptr)
            return E_POINTER;
        *object = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IRawElementProviderSimple))
            *object = static_cast<IRawElementProviderSimple*>(this);
        else if (riid == __uuidof(IRawElementProviderFragment))
            *object = static_cast<IRawElementProviderFragment*>(this);
        else if (riid == __uuidof(IRawElementProviderFragmentRoot) && _host->tree != nullptr
            && _node == _host->tree->root)
            *object = static_cast<IRawElementProviderFragmentRoot*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    // Счётчик атомарный: клиенты UI Automation освобождают элементы из своих потоков
    // The counter is atomic: UI Automation clients release elements from their threads
    ULONG STDMETHODCALLTYPE UiaElement::AddRef()
    {
        return ++_references;
    }

    // Последняя ссылка удаляет объект / The last reference deletes the object
    ULONG STDMETHODCALLTYPE UiaElement::Release()
    {
        ULONG left = --_references;

        if (left == 0)
            delete this;
        return left;
    }

    // Узел ещё в дереве (окно не закрыто, узел не удалён и видим) / The node is still in the tree (the window is open, the node is not removed and visible)
    bool UiaElement::alive() const noexcept
    {
        return _host->tree != nullptr && a11y::contains(*_host->tree, _node);
    }
}
