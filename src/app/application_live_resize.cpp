#include "gran/app/internal/app_state.hpp"
#include "gran/core/i18n.hpp"
#include "gran/log/log.hpp"

#include <chrono>
#include <exception>
#include <string>

namespace gran::app
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        // Кадр перетаскивания дольше этого — в журнал (Т-17: на NVIDIA окно на секунду переставало рисовать)
        // A drag frame longer than this goes to the log (T-17: on NVIDIA the window stopped drawing for a second)
        constexpr double SLOW_LIVE_FRAME = 0.25;

        // Размер бэкенда и компоновка — до синхронного кадра / The backend size and the layout — before the synchronous frame
        void liveFrame(AppWindow& win)
        {
            Size pixels = win.window->pixelSize();

            win.backend->resize(static_cast<std::uint32_t>(pixels.width), static_cast<std::uint32_t>(pixels.height));
            ui::markDirty(*win.document.root, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            renderFrame(win);
        }

        /*
        ** Перерисовка окна во время изменения размера мышью (функтор вместо лямбды); во время замены документа
        ** (COM может доставить WM_SIZE внутри неё) частичное состояние не рисуется. Ошибка кадра здесь не может
        ** уйти дальше оконной процедуры, поэтому она пишется в журнал: раньше такие кадры пропадали молча.
        **
        ** Repainting the window while resizing with the mouse (a functor instead of a lambda); during a document
        ** replacement (COM can deliver WM_SIZE inside it) a partial state is not rendered. A frame error here cannot
        ** go further than the window procedure, so it is written to the log: such frames used to vanish silently.
        */
        struct LiveResize
        {
            AppWindow* win;

            void operator()() const
            {
                if (win->shared.reloading || !win->backend || !win->renderer)
                    return;
                Clock::time_point start = Clock::now();
                try
                {
                    liveFrame(*win);
                }
                catch (const std::exception& error)
                {
                    log::write(log::Level::WARNING, "gpu", i18n::format(i18n::Message::GPU_LIVE_FRAME_FAILED,
                        {error.what()}));
                }
                double spent = std::chrono::duration<double>(Clock::now() - start).count();
                if (spent > SLOW_LIVE_FRAME)
                    log::write(log::Level::WARNING, "gpu", i18n::format(i18n::Message::GPU_LIVE_FRAME_SLOW,
                        {std::to_string(static_cast<int>(spent * 1000.0))}));
            }
        };
    }

    std::function<void()> liveResizeCallback(AppWindow& win)
    {
        return LiveResize{&win};
    }
}
