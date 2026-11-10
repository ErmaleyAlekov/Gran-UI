#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        // Подчинённые закрывающегося окна закрываются с ним (результат ""); владелец стоит в списке раньше — хватает одного прохода
        // The owned windows of a closing window close with it (the result ""); the owner stands earlier in the list — one pass is enough
        void cascadeClosing(Application::State& state) noexcept
        {
            for (const std::unique_ptr<AppWindow>& win : state.windows)
            {
                if (win->closing || win->owner == nullptr || !win->owner->closing)
                    continue;
                win->closing = true;
                win->closeResult.clear();
            }
        }

        /*
        ** Закрыть окно с номером index. Win32: владелец включается ДО уничтожения модального окна,
        ** иначе ОС отдаст фокус чужой программе. Затем снимаются привязки списков и сообщения браузеров,
        ** окно уничтожается (порядок полей AppWindow), и только потом вызывается onClosed — окна уже нет,
        ** обработчик может открыть следующее.
        **
        ** Close the window number index. Win32: the owner is enabled BEFORE the modal window is destroyed,
        ** otherwise the OS gives the focus to a foreign program. Then the list bindings and the browser messages are dropped,
        ** the window is destroyed (the AppWindow field order), and only then onClosed is called — the window is gone,
        ** the handler may open the next one.
        */
        void finishWindow(Application::State& state, std::size_t index)
        {
            AppWindow& win = *state.windows[index];
            AppWindow* owner = win.owner;
            WindowCloseHandler handler = std::move(win.onClosed);
            std::string result = std::move(win.closeResult);

            // Оторванная панель возвращает содержимое домой до того, как окно снимет привязки своего дерева
            // A torn-off panel returns its content home before the window drops the bindings of its tree
            returnPanel(state, win);
            saveState(win);
            win.modal = false;
            updateBlocking(state);
            if (owner != nullptr && !owner->closing && owner->window && !owner->blocked)
                owner->window->activate();
            if (win.document.root)
            {
                state.lists.forget(*win.document.root);
                forgetWebViews(win, *win.document.root);
            }
            state.windows.erase(state.windows.begin() + static_cast<std::ptrdiff_t>(index));
            if (handler)
                handler(result);
        }
    }

    /*
    ** Закрытие главного окна — выход из run() (остальные закроются вместе с приложением). Другие окна
    ** закрываются с конца списка: подчинённые стоят после владельцев и уходят раньше них. Окно,
    ** открытое из onClosed, встаёт в конец и откроется на следующем проходе.
    **
    ** Closing the main window means leaving run() (the others close together with the application). The other windows
    ** close from the list end: the owned ones stand after the owners and go before them. A window
    ** opened from onClosed goes to the end and opens on the next pass.
    */
    void closeWindows(Application::State& state)
    {
        cascadeClosing(state);
        if (!state.windows.empty() && state.windows.front()->closing)
        {
            state.quit = true;
            return;
        }
        for (std::size_t index = state.windows.size(); index > 1; --index)
        {
            // onClosed мог закрыть и другие окна: номер проверяется заново / onClosed could close other windows too: the number is checked again
            if (index - 1 < state.windows.size() && state.windows[index - 1]->closing)
                finishWindow(state, index - 1);
        }
    }
}

namespace gran
{
    /*
    ** Графика окон уходит с конца списка до разрушения документов: окно ОС владельца (Win32) уничтожает
    ** окна подчинённых, и их цепочка обмена не должна пережить свой HWND. Порядок внутри окна — как
    ** у полей AppWindow: рендерер, бэкенд, браузеры, окно ОС.
    **
    ** The window graphics go from the list end before the documents are destroyed: the owner OS window (Win32) destroys
    ** the owned windows, and their swap chain must not outlive its HWND. The order inside a window is as
    ** in the AppWindow fields: the renderer, the backend, the browsers, the OS window.
    */
    Application::State::~State()
    {
        for (std::size_t index = windows.size(); index > 0; --index)
        {
            app::AppWindow& win = *windows[index - 1];

            win.renderer.reset();
            win.backend.reset();
            win.webSlots.clear();
            win.window.reset();
        }
    }
}
