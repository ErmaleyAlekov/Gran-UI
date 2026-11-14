#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>

namespace gran
{
    namespace
    {
        /*
        ** События всех окон, пока хоть одно их получает: разбор сообщений для одного окна ОС
        ** (PeekMessage потока) раскладывает события и в очереди других окон. Обход по индексу:
        ** обработчик может открыть окно. Затем запросы клиентов доступности каждого окна.
        **
        ** The events of all the windows while at least one gets them: dispatching the messages for one OS window
        ** (the thread PeekMessage) puts events into the queues of other windows too. A walk by index:
        ** a handler may open a window. Then the accessibility client requests of every window.
        */
        void pollWindows(Application::State& state)
        {
            platform::Event event;
            bool received = true;

            while (received)
            {
                received = false;
                for (std::size_t index = 0; index < state.windows.size(); ++index)
                {
                    app::AppWindow& win = *state.windows[index];

                    while (win.window && win.window->pollEvent(event))
                    {
                        received = true;
                        win.changed = app::handleEvent(win, event) || win.changed;
                    }
                }
            }
            for (std::size_t index = 0; index < state.windows.size(); ++index)
            {
                app::AppWindow& win = *state.windows[index];

                win.changed = (win.window && app::runAccessRequests(win)) || win.changed;
            }
        }

        // Общее для окон: задачи других потоков, таймеры, сообщения браузеров; true — что-то выполнилось
        // Shared by the windows: the tasks of other threads, the timers, the browser messages; true — something ran
        bool runShared(Application::State& state)
        {
            bool changed = app::runTasks(state);

            changed = state.timers.runDue(app::now()) || changed;
            changed = app::deliverWebMessages(state) || changed;
            // Новый документ главного окна — на границе цикла, после всех обработчиков
            // The new main window document — at a loop boundary, after all the handlers
            app::pollReload(state);
            changed = app::applyReload(state) || changed;
            return changed;
        }

        /*
        ** Кадры окон: окну с событиями, миганием каретки, подсказкой или анимацией; после общих
        ** задач (all) — всем окнам (задача могла изменить любое). true — хоть один кадр нарисован.
        **
        ** The window frames: for a window with events, the caret blink, the tooltip or an animation; after the shared
        ** tasks (all) — for all the windows (a task could change any). true — at least one frame was drawn.
        */
        bool renderWindows(Application::State& state, bool all)
        {
            double time = app::now();
            bool rendered = false;

            for (std::size_t index = 0; index < state.windows.size(); ++index)
            {
                app::AppWindow& win = *state.windows[index];

                if (!win.window)
                    continue;
                win.changed = app::blinkCaret(win, time) || win.changed;
                win.changed = app::updateTooltip(win, time) || win.changed;
                if (all || win.changed || win.animating)
                {
                    app::renderFrame(win);
                    rendered = true;
                }
                win.changed = false;
            }
            return rendered;
        }

        // Сон до ближайшего срока: окна (каретка, подсказка), таймеры, выход по времени; отрицательное — без предела
        // The sleep until the nearest due time: the windows (the caret, the tooltip), the timers, the timed exit; negative — no limit
        double sleepTime(const Application::State& state, double time)
        {
            // Срок таймера 0 возможен, выход по времени 0 — «не выходить» / A timer due time of 0 is possible, a timed exit of 0 means "never"
            double waits[3] = {state.timers.nextDue(), -1.0, -1.0};
            double sleep = -1.0;

            if (state.options.exitAfter > 0.0)
                waits[1] = state.options.exitAfter;
            if (app::reloadWait(state, time) >= 0.0)
                waits[2] = state.nextReloadCheck;
            for (const std::unique_ptr<app::AppWindow>& win : state.windows)
            {
                double wait = -1.0;

                if (win->window)
                    wait = app::windowSleep(*win, time);
                if (wait >= 0.0 && (sleep < 0.0 || wait < sleep))
                    sleep = wait;
            }
            for (double due : waits)
            {
                if (due >= 0.0 && (sleep < 0.0 || std::max(0.0, due - time) < sleep))
                    sleep = std::max(0.0, due - time);
            }
            return sleep;
        }
    }

    /*
    ** Шаг цикла: события всех окон; общие задачи, таймеры и сообщения браузеров; закрытие окон;
    ** новые окна получают графику и первый кадр; кадры тем окнам, где что-то изменилось или идёт анимация
    ** (vsync задаёт темп). Ничего не нарисовано и wait — сон до события ОС или ближайшего срока.
    ** Закрытие главного окна завершает цикл.
    **
    ** A loop step: the events of all the windows; the shared tasks, the timers and the browser messages; closing the windows;
    ** new windows get their graphics and the first frame; frames for the windows where something changed or an animation runs
    ** (vsync sets the pace). Nothing drawn and wait — sleeping until an OS event or the nearest due time.
    ** Closing the main window ends the loop.
    */
    app::LoopStep app::runLoopStep(Application::State& state, bool wait)
    {
        pollWindows(state);
        bool shared = runShared(state);
        if (state.options.exitAfter > 0.0 && app::now() >= state.options.exitAfter)
            state.quit = true;
        app::closeWindows(state);
        if (state.quit)
            return LoopStep::QUIT;
        app::openWindows(state);
        if (renderWindows(state, shared))
            return LoopStep::BUSY;
        if (wait)
            state.windows.front()->window->waitEvents(sleepTime(state, app::now()));
        return LoopStep::IDLE;
    }
}
