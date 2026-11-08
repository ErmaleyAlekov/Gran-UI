#include "render_tests.hpp"
#include "gran/app/application.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>
#include <iostream>
#include <memory>

namespace render_tests
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        // Число шагов «перетаскивания края» и допустимое время разбора очереди после него; кадры — без vsync,
        // чтобы 300 синхронных кадров не растянули тест. Запасной выход — через 20 с.
        // The number of "edge drag" steps and the allowed time to drain the queue after it; the frames go without
        // vsync so that 300 synchronous frames do not stretch the test. The fallback exit is after 20 s.
        constexpr int STORM_STEPS = 300;
        constexpr double DRAIN_LIMIT = 0.05;

        /*
        ** Перетаскивание края окна без мыши: WM_ENTERSIZEMOVE, затем серия SetWindowPos (каждый даёт
        ** синхронное WM_SIZE с кадром LiveResize и событие RESIZE в очереди), затем WM_EXITSIZEMOVE.
        ** Так же ведёт себя модальный цикл Windows, пока пользователь тянет край.
        **
        ** Dragging the window edge without a mouse: WM_ENTERSIZEMOVE, then a series of SetWindowPos (each gives
        ** a synchronous WM_SIZE with a LiveResize frame and a RESIZE event in the queue), then WM_EXITSIZEMOVE.
        ** The Windows modal loop behaves the same way while the user drags the edge.
        */
        bool storm(HWND window)
        {
            RECT start{};

            if (window == nullptr || !GetWindowRect(window, &start))
                return false;
            SendMessageW(window, WM_ENTERSIZEMOVE, 0, 0);
            for (int step = 0; step < STORM_STEPS; ++step)
            {
                int width = (start.right - start.left) + (step % 60) * 6;
                int height = (start.bottom - start.top) + (step % 40) * 5;
                SetWindowPos(window, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            SendMessageW(window, WM_EXITSIZEMOVE, 0, 0);
            return true;
        }

        // Состояние проверки, общее для копий функтора таймера.
        // The check state shared by the copies of the timer functor.
        struct ResizeState
        {
            unsigned phase = 0;
            bool stormed = false;
            Clock::time_point after;
            double drain = -1.0;
        };

        /*
        ** Таймер: тик «перетаскивания» (пока окно не найдено — ждём следующего тика), затем тик замера:
        ** сколько цикл разбирал очередь после WM_EXITSIZEMOVE; после замера окно закрывается.
        **
        ** The timer: the "drag" tick (while the window is not found — wait for the next tick), then the measuring
        ** tick: how long the loop drained the queue after WM_EXITSIZEMOVE; after measuring the window closes.
        */
        struct ResizeProbe
        {
            gran::Application* app;
            std::shared_ptr<ResizeState> state;

            void operator()() const
            {
                if (state->phase == 0)
                {
                    state->stormed = storm(FindWindowW(nullptr, L"gran-resize-live"));
                    state->after = Clock::now();
                    if (!state->stormed)
                        return;
                }
                else if (state->phase == 1)
                {
                    state->drain = std::chrono::duration<double>(Clock::now() - state->after).count();
                    app->closeWindow(*app->find("storm"));
                }
                ++state->phase;
            }
        };
    }

    /*
    ** Настоящее окно и GPU: после серии изменений размера внутри модального цикла главный цикл должен
    ** вернуться к работе сразу. Регрессия Т-14: каждое устаревшее событие RESIZE пересоздавало цепочку
    ** обмена, и окно зависало на секунды.
    **
    ** A real window and GPU: after a series of size changes inside the modal loop the main loop must
    ** get back to work at once. The T-14 regression: every stale RESIZE event recreated the swap chain,
    ** and the window froze for seconds.
    */
    bool runResize(const Options& options)
    {
        gran::AppOptions settings;
        settings.backend = options.backend;
        settings.forceWarp = options.warp;
        settings.vsync = false;
        settings.exitAfter = 20.0;
        gran::Application app(settings);
        auto state = std::make_shared<ResizeState>();

        app.loadString("<Window title=\"gran-resize-live\" width=\"480\" height=\"320\">"
            "<Text id=\"storm\">resize storm</Text></Window>");
        app.setInterval(0.05, ResizeProbe{&app, state});
        app.run();
        bool passed = state->stormed && state->drain >= 0.0 && state->drain < DRAIN_LIMIT;
        if (passed)
            std::cout << "[ OK ] live resize storm, drain " << state->drain << " s\n";
        else
            std::cout << "[FAIL] live resize storm, stormed=" << state->stormed << " drain " << state->drain << " s\n";
        return passed;
    }
}
