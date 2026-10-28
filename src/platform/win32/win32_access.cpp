#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Связь создаётся один раз; дерево можно заменить (новая разметка) или отключить (nullptr).
    ** Вход в апартамент COM — как у WebView2 (тот же поток, тот же STA); парный CoUninitialize
    ** не вызывается: клиенты UI Automation отпускают провайдеры асинхронно, до конца процесса.
    ** RPC_E_CHANGED_MODE (поток уже в многопоточном апартаменте) — доступность недоступна.
    **
    ** The link is created once; the tree can be replaced (new markup) or disconnected (nullptr).
    ** Entering the COM apartment is as for WebView2 (the same thread, the same STA); the matching CoUninitialize
    ** is not called: UI Automation clients release the providers asynchronously, until the process ends.
    ** RPC_E_CHANGED_MODE (the thread is already in a multithreaded apartment) — accessibility is unavailable.
    */
    void Win32Window::setAccessTree(a11y::AccessTree* tree)
    {
        // Старые провайдеры должны остаться отключёнными даже после следующей загрузки.
        // Old providers must remain disconnected even after the next load.
        if (tree == nullptr)
        {
            if (access)
                access->tree = nullptr;
            access.reset();
            return;
        }
        if (!access)
        {
            // Вход в COM один раз на поток, без накопления счётчика при перезагрузках.
            // Enter COM once per thread without accumulating its counter on reloads.
            static thread_local const bool APARTMENT = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            access = std::make_shared<UiaHost>();
            access->apartment = APARTMENT;
        }
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

    // Изменения отметки, текста и числа узла в фокусе — дикторам (сверка — uia_events.cpp)
    // The changes of the focused node mark, text and number go to the screen readers (the comparison is in uia_events.cpp)
    void Win32Window::accessRefresh()
    {
        if (access)
            raiseStateChanges(access);
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
        if (static_cast<long>(lParam) != UiaRootObjectId || !self.access || self.access->tree == nullptr
            || !self.access->apartment)
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
