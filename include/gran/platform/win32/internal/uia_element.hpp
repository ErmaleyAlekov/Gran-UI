#ifndef GRAN_PLATFORM_WIN32_INTERNAL_UIA_ELEMENT_HPP
#define GRAN_PLATFORM_WIN32_INTERNAL_UIA_ELEMENT_HPP

#include "gran/a11y/access_tree.hpp"
#include "gran/platform/win32/win32_window.hpp"

#include <ole2.h>
#include <uiautomation.h>

#include <atomic>
#include <memory>

namespace gran::platform::win32
{
    /*
    ** Общая для всех элементов окна связь с деревом. Элементы держат её через shared_ptr:
    ** окно закрылось — tree обнуляется, а элементы, ещё живые у клиентов, отвечают
    ** UIA_E_ELEMENTNOTAVAILABLE вместо обращения к удалённым узлам.
    **
    ** The link to the tree shared by all window elements. The elements hold it through shared_ptr:
    ** the window closed — tree is reset, and the elements still alive at the clients answer
    ** UIA_E_ELEMENTNOTAVAILABLE instead of touching removed nodes.
    */
    struct UiaHost
    {
        HWND window = nullptr;
        a11y::AccessTree* tree = nullptr;
    };

    /*
    ** Элемент UI Automation для одного узла (паттерн Adapter над a11y::describe и a11y::navigate).
    ** Один класс реализует простой провайдер, фрагмент и (для корня дерева) корень фрагментов.
    ** Элементы создаются по запросу: UI Automation сравнивает их по RuntimeId, а не по адресу.
    ** Провайдер не свободнопоточный — UI Automation вызывает его в потоке окна, где живёт дерево.
    **
    ** A UI Automation element for one node (the Adapter pattern over a11y::describe and a11y::navigate).
    ** One class implements the simple provider, the fragment and (for the tree root) the fragment root.
    ** The elements are created on request: UI Automation compares them by RuntimeId, not by address.
    ** The provider is not free-threaded — UI Automation calls it on the window thread where the tree lives.
    */
    class UiaElement final : public IRawElementProviderSimple, public IRawElementProviderFragment,
                             public IRawElementProviderFragmentRoot
    {
    public:
        // ---- uia_element.cpp ----
        UiaElement(std::shared_ptr<UiaHost> host, const ui::Node* node);

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        // Узел жив и дерево подключено / The node is alive and the tree is connected
        bool alive() const noexcept;

        // ---- uia_simple.cpp ----
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* options) override;
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID pattern, IUnknown** provider) override;
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID property, VARIANT* value) override;
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** provider) override;

        // ---- uia_fragment.cpp ----
        HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** result) override;
        HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** id) override;
        HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* rect) override;
        HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** roots) override;
        HRESULT STDMETHODCALLTYPE SetFocus() override;

        // ---- uia_root.cpp ----
        HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** root) override;
        HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** result) override;
        HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** result) override;

    private:
        ~UiaElement() = default;

        std::shared_ptr<UiaHost> _host;
        const ui::Node* _node;
        std::atomic<ULONG> _references{1};
    };

    // ---- uia_root.cpp ----

    // Новый элемент узла (ссылка 1 — её забирает вызывающий) или nullptr / A new node element (reference 1 — the caller takes it) or nullptr
    UiaElement* elementFor(const std::shared_ptr<UiaHost>& host, const ui::Node* node) noexcept;

    // ---- uia_properties.cpp ----

    // Значение свойства узла; false — свойство не поддержано (VT_EMPTY) / The node property value; false — not supported (VT_EMPTY)
    bool nodeProperty(const UiaHost& host, const ui::Node& node, PROPERTYID property, VARIANT* value);

    // Прямоугольник узла в пикселях экрана / The node rectangle in screen pixels
    UiaRect screenRect(const UiaHost& host, const ui::Node& node) noexcept;
}

#endif
