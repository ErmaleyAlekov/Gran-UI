#ifndef GRAN_PLATFORM_WIN32_INTERNAL_UIA_ELEMENT_HPP
#define GRAN_PLATFORM_WIN32_INTERNAL_UIA_ELEMENT_HPP

#include "gran/a11y/access_tree.hpp"
#include "gran/platform/win32/win32_window.hpp"

#include <ole2.h>
#include <uiautomation.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

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
    struct UiaHost;

    /*
    ** Прошлое состояние узла в фокусе: с ним сверяется новое, разница уходит дикторам событиями
    ** изменения свойств. Указатель node только сравнивается с фокусом и не разыменовывается.
    **
    ** The former focused node state: the new one is compared with it, the difference goes to the screen readers as
    ** property change events. The node pointer is only compared with the focus and is never dereferenced.
    */
    struct AccessSnapshot
    {
        const ui::Node* node = nullptr;
        bool checked = false;
        float current = 0.0f;
        std::string value;
    };

    /*
    ** apartment — поток окна вошёл в однопоточный апартамент COM: только тогда UI Automation
    ** (с ProviderOptions_UseComThreading) вызывает провайдер в потоке окна через его очередь сообщений.
    ** Без апартамента вызовы шли бы из потоков RPC параллельно с изменением дерева — дерево не отдаётся.
    **
    ** apartment — the window thread entered a COM single-threaded apartment: only then UI Automation
    ** (with ProviderOptions_UseComThreading) calls the provider on the window thread through its message queue.
    ** Without the apartment the calls would come from RPC threads in parallel with tree changes — the tree is not given out.
    */
    struct UiaHost
    {
        HWND window = nullptr;
        a11y::AccessTree* tree = nullptr;
        AccessSnapshot watched;
        bool apartment = false;
    };

    /*
    ** Элемент UI Automation для одного узла (паттерн Adapter над a11y::describe и a11y::navigate).
    ** Один класс реализует простой провайдер, фрагмент, (для корня дерева) корень фрагментов и шаблоны
    ** управления: GetPatternProvider отдаёт сам элемент, если узел поддерживает шаблон (a11y::patternsOf).
    ** Действия шаблонов не меняют дерево, а ставят запрос в очередь a11y::AccessTree.
    ** Элементы создаются по запросу: UI Automation сравнивает их по RuntimeId, а не по адресу.
    ** Провайдер не свободнопоточный — UI Automation вызывает его в потоке окна, где живёт дерево.
    **
    ** A UI Automation element for one node (the Adapter pattern over a11y::describe and a11y::navigate).
    ** One class implements the simple provider, the fragment, (for the tree root) the fragment root and the control
    ** patterns: GetPatternProvider gives the element itself if the node supports the pattern (a11y::patternsOf).
    ** The pattern actions do not change the tree, they queue a request into a11y::AccessTree.
    ** The elements are created on request: UI Automation compares them by RuntimeId, not by address.
    ** The provider is not free-threaded — UI Automation calls it on the window thread where the tree lives.
    */
    class UiaElement final : public IRawElementProviderSimple, public IRawElementProviderFragment,
                             public IRawElementProviderFragmentRoot, public IInvokeProvider, public IToggleProvider,
                             public IValueProvider, public IRangeValueProvider, public ISelectionItemProvider
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

        // ---- uia_invoke.cpp: Invoke и Toggle / Invoke and Toggle ----
        HRESULT STDMETHODCALLTYPE Invoke() override;
        HRESULT STDMETHODCALLTYPE Toggle() override;
        HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* state) override;

        // ---- uia_value.cpp: Value; get_IsReadOnly общий с RangeValue / Value; get_IsReadOnly is shared with RangeValue ----
        HRESULT STDMETHODCALLTYPE SetValue(LPCWSTR text) override;
        HRESULT STDMETHODCALLTYPE get_Value(BSTR* text) override;
        HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* readOnly) override;
        HRESULT STDMETHODCALLTYPE get_LargeChange(double* step) override;

        // ---- uia_range.cpp: RangeValue ----
        HRESULT STDMETHODCALLTYPE SetValue(double value) override;
        HRESULT STDMETHODCALLTYPE get_Value(double* value) override;
        HRESULT STDMETHODCALLTYPE get_Maximum(double* maximum) override;
        HRESULT STDMETHODCALLTYPE get_Minimum(double* minimum) override;
        HRESULT STDMETHODCALLTYPE get_SmallChange(double* step) override;

        // ---- uia_selection.cpp: SelectionItem ----
        HRESULT STDMETHODCALLTYPE Select() override;
        HRESULT STDMETHODCALLTYPE AddToSelection() override;
        HRESULT STDMETHODCALLTYPE RemoveFromSelection() override;
        HRESULT STDMETHODCALLTYPE get_IsSelected(BOOL* selected) override;
        HRESULT STDMETHODCALLTYPE get_SelectionContainer(IRawElementProviderSimple** container) override;

        // ---- uia_patterns.cpp ----

        // Интерфейс шаблона pattern (UIA_*PatternId), если узел его поддерживает; иначе nullptr / The pattern interface (UIA_*PatternId) if the node supports it; otherwise nullptr
        IUnknown* patternFor(PATTERNID pattern) noexcept;

        // Интерфейс шаблона по его IID (для QueryInterface); nullptr — не шаблон или не поддержан / The pattern interface by its IID (for QueryInterface); nullptr — not a pattern or not supported
        IUnknown* patternByIid(REFIID riid) noexcept;

    private:
        ~UiaElement() = default;

        // ---- uia_patterns.cpp ----

        // Узел жив и поддерживает шаблон (биты a11y::Pattern): S_OK или код ошибки UI Automation
        // The node is alive and supports the pattern (a11y::Pattern bits): S_OK or a UI Automation error code
        HRESULT check(std::uint8_t pattern) const noexcept;

        // Запрос в очередь дерева; отключённый узел — UIA_E_ELEMENTNOTENABLED / A request into the tree queue; a disabled node — UIA_E_ELEMENTNOTENABLED
        HRESULT post(const a11y::AccessRequest& request) noexcept;

        // Число диапазона из a11y::describe (поле field) / A range number from a11y::describe (the field)
        HRESULT rangeNumber(double* result, float a11y::AccessibleInfo::* field) const noexcept;

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

    // ---- uia_events.cpp ----

    // Сверка узла в фокусе с прошлым состоянием и события изменения свойств / Comparing the focused node with the former state and the property change events
    void raiseStateChanges(const std::shared_ptr<UiaHost>& host) noexcept;
}

#endif
