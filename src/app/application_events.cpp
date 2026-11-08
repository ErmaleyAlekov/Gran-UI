#include "gran/app/internal/app_state.hpp"

#include <algorithm>

namespace gran::app
{
    namespace
    {
        // Период мигания каретки (как в Windows по умолчанию) / The caret blink period (as the Windows default)
        constexpr double CARET_PERIOD = 0.53;

        /*
        ** Попытка закрыть окно (крестик, Alt+F4). Без onClose окно закрывается после прохода событий.
        ** С onClose решает обработчик (Application::closeWindow или ничего — окно остаётся), но только
        ** если он зарегистрирован: иначе окно нельзя было бы закрыть вовсе.
        **
        ** An attempt to close the window (the cross, Alt+F4). Without onClose the window closes after the event pass.
        ** With onClose the handler decides (Application::closeWindow or nothing — the window stays), but only
        ** if it is registered: otherwise the window could not be closed at all.
        */
        void requestClose(AppWindow& win)
        {
            const std::string& name = win.document.window.onClose;
            auto handler = win.shared.actions.find(name);

            if (name.empty() || !win.document.root || handler == win.shared.actions.end() || !handler->second)
            {
                win.closing = true;
                return;
            }
            win.action(name, *win.document.root);
        }

        /*
        ** Размер цепочки обмена — по нынешнему размеру окна, а не по размеру из события. Пока край окна тянут
        ** мышью, Windows крутит свой модальный цикл: каждое WM_SIZE уже перерисовано синхронно (LiveResize),
        ** но его событие остаётся в очереди. После отпускания кнопки цикл получает сотни устаревших RESIZE
        ** подряд; по размеру из события каждое стоило бы ожидания GPU и ResizeBuffers до уже ненужного
        ** размера — окно зависало на секунды, по краям мелькал мусор (найдено в ручной проверке Т-14 на
        ** Intel и NVIDIA). По нынешнему размеру все, кроме первого, пропускаются бэкендом как «тот же размер».
        **
        ** The swap chain size follows the current window size, not the size from the event. While the window edge
        ** is dragged with the mouse, Windows runs its own modal loop: every WM_SIZE is already repainted
        ** synchronously (LiveResize), but its event stays in the queue. After the button is released the loop gets
        ** hundreds of stale RESIZE events in a row; by the event size each would cost a GPU wait and ResizeBuffers
        ** to a size no longer needed — the window froze for seconds and garbage flashed at the edges (found in the
        ** manual check T-14 on Intel and NVIDIA). By the current size all but the first are skipped by the backend
        ** as "the same size".
        */
        void resizeToWindow(AppWindow& win)
        {
            Size pixels = win.window->pixelSize();

            win.backend->resize(static_cast<std::uint32_t>(pixels.width), static_cast<std::uint32_t>(pixels.height));
        }
    }

    // Размер, DPI, перерисовка — графике; закрытие — requestClose; остальное — маршрутизатору ввода
    // The size, DPI, repaint go to the graphics; closing — to requestClose; the rest — to the input router
    bool handleEvent(AppWindow& win, const platform::Event& event)
    {
        // Окно под модальным: ввод не доходит до дерева / A window under a modal one: the input does not reach the tree
        if (dropBlockedInput(win, event))
            return false;
        switch (event.kind)
        {
            case platform::EventKind::CLOSE: requestClose(win); return false;
            case platform::EventKind::RESIZE: resizeToWindow(win); break;
            case platform::EventKind::DPI_CHANGED: break;
            case platform::EventKind::EXPOSE: return true;
            case platform::EventKind::DRAG_MOVE: case platform::EventKind::DRAG_LEAVE:
            case platform::EventKind::DROP: return handleDrag(win, event);
            default:
                if (event.kind == platform::EventKind::MOUSE_DOWN || event.kind == platform::EventKind::KEY_DOWN
                    || event.kind == platform::EventKind::MOUSE_WHEEL)
                    hideTooltip(win, true);
                win.caretVisible = true;
                win.caretToggle = now() + CARET_PERIOD;
                return win.router->handle(event);
        }
        ui::markDirty(*win.document.root, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        return true;
    }

    // Мигание каретки поля в фокусе этого окна / The caret blink of the focused field of this window
    bool blinkCaret(AppWindow& win, double time)
    {
        const ui::Node* focus = win.router->focused();

        if (focus == nullptr || !ui::isTextField(*focus) || time < win.caretToggle)
            return false;
        win.caretVisible = !win.caretVisible;
        win.caretToggle = time + CARET_PERIOD;
        return true;
    }

    /*
    ** Сон окна: до мигания каретки или появления подсказки. Запрос доступности, пришедший во время
    ** кадра или задач (исходящий вызов COM принимает входящие), — сна нет.
    **
    ** The window sleep: until the caret blink or the tooltip appearance. An accessibility request that came during
    ** the frame or the tasks (an outgoing COM call accepts incoming ones) — no sleep.
    */
    double windowSleep(const AppWindow& win, double time)
    {
        const ui::Node* focus = win.router->focused();
        double tooltip = tooltipWait(win, time);
        double caret = -1.0;

        if (!win.access.requests.empty())
            return 0.0;
        if (focus != nullptr && ui::isTextField(*focus))
            caret = std::max(0.0, win.caretToggle - time);
        if (caret < 0.0 || (tooltip >= 0.0 && tooltip < caret))
            return tooltip;
        return caret;
    }
}
