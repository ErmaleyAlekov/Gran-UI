#include "render_tests.hpp"
#include "gran/app/application.hpp"
// windows.h — после заголовков движка: его макросы ломают их объявления
// windows.h — after the engine headers: its macros break their declarations
#include "resize_drag.hpp"
#include "resize_duplication.hpp"

#include <iostream>
#include <memory>

namespace render_tests
{
    namespace
    {
        /*
        ** Темп «дёрганья» края, при котором мусор виден на всех картах стенда: 40 шагов мыши по 20 пикселей
        ** через 4 мс (250 Гц, как у обычной мыши), три раза вправо-влево. Без исправления Т-15 — Intel D3D12
        ** 120 плохих кадров из 376, Intel D3D11 — 88, NVIDIA — около 30; рост в одну сторону мусора почти не даёт.
        **
        ** The edge "jerking" pace at which the garbage shows on all stand cards: 40 mouse steps of 20 pixels
        ** 4 ms apart (250 Hz, as an ordinary mouse), three times right and left. Without the T-15 fix — Intel D3D12
        ** 120 bad frames out of 376, Intel D3D11 — 88, NVIDIA — about 30; growing one way gives almost no garbage.
        */
        constexpr int GROW_STEPS = 40;
        constexpr int GROW_STEP = 20;
        constexpr DWORD STEP_GAP_MS = 4;
        constexpr int ROUNDS = 3;
        constexpr DWORD SETTLE_MS = 300;

        // Состояние замера между тиками таймера / The measurement state between the timer ticks
        struct ArtifactStats
        {
            unsigned phase = 0;
            bool stormed = false;
            HWND backdrop = nullptr;
            FrameWatcher watcher;
            std::shared_ptr<DragState> drag = std::make_shared<DragState>();
            WatchResult watch;
        };

        // Клиентская область окна в координатах экрана / The window client area in screen coordinates
        RECT clientOnScreen(HWND window)
        {
            RECT area{};

            GetClientRect(window, &area);
            MapWindowPoints(window, nullptr, reinterpret_cast<POINT*>(&area), 2);
            return area;
        }

        /*
        ** Начало: серая подложка под окном, наблюдатель за полосой, которую окно займёт (от нынешнего правого
        ** края до конечного), и поток мыши. Дальше поток окна уходит в модальный цикл Windows.
        **
        ** The start: a gray backdrop below the window, a watcher of the band the window will take (from the current
        ** right edge to the final one) and the mouse thread. Then the window thread goes into the Windows modal loop.
        */
        bool beginDrag(HWND window, ArtifactStats& stats)
        {
            RECT start{};
            DragPlan& plan = stats.drag->plan;

            if (window == nullptr || !GetWindowRect(window, &start))
                return false;
            SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            if (!resizeCorner(window, plan.corner))
                return false;
            plan.steps = GROW_STEPS;
            plan.step = GROW_STEP;
            plan.gapMs = STEP_GAP_MS;
            plan.rounds = ROUNDS;
            RECT client = clientOnScreen(window);
            RECT area{start.left, start.top, start.right + GROW_STEPS * GROW_STEP + 40, start.bottom + 40};
            stats.backdrop = createBackdrop(window, area);
            stats.watcher.start(RECT{client.right, client.top, client.right + GROW_STEPS * GROW_STEP, client.bottom});
            startDrag(stats.drag);
            return true;
        }

        // Конец: кнопка отпущена — дождаться последних кадров и остановить наблюдателя
        // The end: the button is released — wait for the last frames and stop the watcher
        void finishDrag(ArtifactStats& stats)
        {
            stats.drag->thread.join();
            Sleep(SETTLE_MS);
            stats.watch = stats.watcher.stop();
            DestroyWindow(stats.backdrop);
        }

        // Таймер: начало, ожидание отпускания кнопки, конец и закрытие окна (функтор вместо лямбды)
        // The timer: the start, waiting for the button release, the end and closing the window (a functor instead of a lambda)
        struct ArtifactProbe
        {
            std::shared_ptr<ArtifactStats> stats;

            void operator()() const
            {
                if (stats->phase == 0 && beginDrag(threadWindow(), *stats))
                    stats->phase = 1;
                else if (stats->phase == 1 && stats->drag->done.load())
                {
                    finishDrag(*stats);
                    stats->stormed = true;
                    stats->phase = 2;
                    // WM_CLOSE — как крестик: закрытие главного окна завершает run()
                    // WM_CLOSE — like the cross: closing the main window ends run()
                    PostMessageW(threadWindow(), WM_CLOSE, 0, 0);
                }
            }
        };
    }

    /*
    ** Мусор на краях при перетаскивании (Т-15): сколько кадров монитора за время роста окна показали белую
    ** или чёрную полосу вместо содержимого окна. Окно — витрина (samples/showcase.xml, тяжёлый кадр, как у
    ** пользователя) или разметка из --markup; подложка серая, поэтому белое и чёрное — только непрорисованная
    ** часть окна. Кадры — с vsync, как у приложений. Двигает настоящий курсор и снимает монитор: запускать
    ** в сеансе с рабочим столом, где никто не работает (стенд).
    **
    ** Garbage at the edges while dragging (T-15): how many monitor frames during the window growth showed a white
    ** or black strip instead of the window content. The window is the showcase (samples/showcase.xml, a heavy frame,
    ** as the user has) or the markup from --markup; the backdrop is gray, so white and black are only the unpainted
    ** part of the window. The frames go with vsync, as in applications. It moves the real cursor and captures the
    ** monitor: run it in a session with a desktop where nobody works (the stand).
    */
    bool runResizeArtifacts(const Options& options)
    {
        gran::AppOptions settings;
        settings.backend = options.backend;
        settings.forceWarp = options.warp;
        settings.exitAfter = 30.0;
        gran::Application app(settings);
        auto stats = std::make_shared<ArtifactStats>();

        app.loadFile(options.markup);
        app.setInterval(0.25, ArtifactProbe{stats});
        app.run();
        const WatchResult& watch = stats->watch;
        bool passed = stats->stormed && watch.opened && watch.frames > 0 && watch.badFrames == 0;
        if (passed)
            std::cout << "[ OK ] ";
        else
            std::cout << "[FAIL] ";
        std::cout << "resize artifacts: " << watch.badFrames << "/" << watch.frames
                  << " monitor frames with a white/black strip, widest " << watch.worstColumns << " px"
                  << " (stormed " << stats->stormed << ", duplication " << watch.opened << ")\n";
        return passed;
    }
}
