#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    // Связь создаётся один раз; дерево можно заменить (новая разметка) или отключить (nullptr)
    // The link is created once; the tree can be replaced (new markup) or disconnected (nullptr)
    void Win32Window::setAccessTree(a11y::AccessTree* tree)
    {
        if (!access)
            access = std::make_shared<UiaHost>();
        access->window = window;
        access->tree = tree;
    }

    // Событие фокуса — только если кто-то слушает (без клиентов вызов UI Automation лишний)
    // The focus event — only if someone listens (without clients the UI Automation call is wasted)
    void Win32Window::accessFocusChanged()
    {
        if (!access || access->tree == nullptr || access->tree->focus == nullptr || !UiaClientsAreListening())
            return;
        UiaElement* element = elementFor(access, access->tree->focus);
        if (element == nullptr)
            return;
        // Ошибка рассылки не мешает работе окна: клиент спросит фокус сам через GetFocus
        // A broadcast error does not hinder the window: the client asks for the focus itself through GetFocus
        UiaRaiseAutomationEvent(element, UIA_AutomationFocusChangedEventId);
        element->Release();
    }

    /*
    ** WM_GETOBJECT: на запрос корня UI Automation окно отдаёт элемент корня дерева; прочие
    ** запросы (MSAA, OBJID_CLIENT) — стандартной обработке, она построит прокси сама.
    **
    ** WM_GETOBJECT: on the UI Automation root request the window gives the tree root element; other
    ** requests (MSAA, OBJID_CLIENT) go to the default handling, it builds a proxy itself.
    */
    LRESULT answerAccessRequest(Win32Window& self, WPARAM wParam, LPARAM lParam) noexcept
    {
        if (static_cast<long>(lParam) != UiaRootObjectId || !self.access || self.access->tree == nullptr)
            return DefWindowProcW(self.window, WM_GETOBJECT, wParam, lParam);
        UiaElement* root = elementFor(self.access, self.access->tree->root);
        if (root == nullptr)
            return DefWindowProcW(self.window, WM_GETOBJECT, wParam, lParam);
        LRESULT result = UiaReturnRawElementProvider(self.window, wParam, lParam, root);
        root->Release();
        return result;
    }

    // Окно закрывается: живые у клиентов элементы теряют дерево, UI Automation отпускает провайдеры
    // The window closes: the elements alive at the clients lose the tree, UI Automation releases the providers
    void disconnectAccess(Win32Window& self) noexcept
    {
        if (!self.access)
            return;
        self.access->tree = nullptr;
        UiaReturnRawElementProvider(self.window, 0, 0, nullptr);
    }
}
