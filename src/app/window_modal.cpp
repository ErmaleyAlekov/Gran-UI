#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    namespace
    {
        // Принадлежит ли win окну owner (напрямую или через цепочку владельцев) / Whether win belongs to owner (directly or through the owner chain)
        bool ownedBy(const AppWindow& win, const AppWindow& owner) noexcept
        {
            for (const AppWindow* step = win.owner; step != nullptr; step = step->owner)
            {
                if (step == &owner)
                    return true;
            }
            return false;
        }

        // Ввод пользователя (мышь, клавиатура, перетаскивание, закрытие), а не служебное событие окна
        // A user input (the mouse, the keyboard, dragging, closing), not a service window event
        bool isUserInput(platform::EventKind kind) noexcept
        {
            return kind != platform::EventKind::RESIZE && kind != platform::EventKind::DPI_CHANGED
                && kind != platform::EventKind::EXPOSE && kind != platform::EventKind::FOCUS_GAINED
                && kind != platform::EventKind::FOCUS_LOST;
        }
    }

    /*
    ** Модальное окно, которое блокирует win: последнее открытое модальное среди подчинённых win
    ** (последнее в списке — верхнее; именно его нужно поднять по щелчку). nullptr — win свободно.
    **
    ** The modal window that blocks win: the last opened modal one among the windows owned by win
    ** (the last in the list is the top one; it is the one to raise on a click). nullptr — win is free.
    */
    AppWindow* blockingModal(Application::State& state, const AppWindow& win) noexcept
    {
        for (std::size_t index = state.windows.size(); index > 0; --index)
        {
            AppWindow& candidate = *state.windows[index - 1];

            if (candidate.modal && ownedBy(candidate, win))
                return &candidate;
        }
        return nullptr;
    }

    /*
    ** Пересчёт блокировки после открытия и закрытия окон. Окно ОС выключается (Win32) и теряет
    ** наведение и нажатие (уход мыши): иначе кнопка под мышью осталась бы подсвеченной.
    ** Окно без окна ОС (ещё не показано) только запоминает блокировку.
    **
    ** Recomputing the blocking after opening and closing windows. The OS window is disabled (Win32) and loses
    ** the hover and the press (the mouse leaving): otherwise the button under the mouse would stay highlighted.
    ** A window without an OS window (not shown yet) only remembers the blocking.
    */
    void updateBlocking(Application::State& state)
    {
        for (const std::unique_ptr<AppWindow>& win : state.windows)
        {
            bool blocked = blockingModal(state, *win) != nullptr;

            if (blocked == win->blocked)
                continue;
            win->blocked = blocked;
            if (win->window)
                win->window->setEnabled(!blocked);
            if (blocked && win->router)
            {
                platform::Event leave;
                leave.kind = platform::EventKind::MOUSE_LEAVE;
                win->changed = win->router->handle(leave) || win->changed;
            }
        }
    }

    /*
    ** Ввод заблокированного окна отбрасывается (на X11 ОС его не задерживает). Нажатие, клавиша, попытка
    ** закрыть или фокус (X11 отдаёт фокус и выключенному окну) поднимают модальное окно. true — событие поглощено.
    **
    ** The input of a blocked window is dropped (on X11 the OS does not hold it back). A press, a key, an attempt
    ** to close or the focus (X11 gives the focus to a disabled window too) raise the modal window. true — the event is consumed.
    */
    bool dropBlockedInput(AppWindow& win, const platform::Event& event)
    {
        platform::EventKind kind = event.kind;
        AppWindow* modal = nullptr;

        if (!win.blocked)
            return false;
        if (kind == platform::EventKind::MOUSE_DOWN || kind == platform::EventKind::KEY_DOWN
            || kind == platform::EventKind::CLOSE || kind == platform::EventKind::FOCUS_GAINED)
            modal = blockingModal(win.shared, win);
        if (modal != nullptr && modal->window)
            modal->window->activate();
        return isUserInput(kind);
    }
}
