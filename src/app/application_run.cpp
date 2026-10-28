#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>

namespace gran
{
    namespace
    {
        // Перерисовка во время изменения размера мышью (функтор вместо лямбды) / Repainting while resizing with the mouse (a functor instead of a lambda)
        struct LiveResize
        {
            Application::State* state;

            void operator()() const
            {
                app::renderFrame(*state);
            }
        };

        /*
        ** Окно будится задачами других потоков только внутри run(): при выходе (и по исключению)
        ** получатель снимается до того, как State разрушит окно.
        **
        ** The window is woken by the tasks of other threads only inside run(): on leaving (and on an exception)
        ** the target is removed before State destroys the window.
        */
        struct WakeScope
        {
            Application::State& state;

            explicit WakeScope(Application::State& owner) : state(owner)
            {
                app::setWakeTarget(state, state.window.get());
            }

            ~WakeScope()
            {
                app::setWakeTarget(state, nullptr);
            }

            WakeScope(const WakeScope&) = delete;
            WakeScope& operator=(const WakeScope&) = delete;
        };

        // Период мигания каретки (как в Windows по умолчанию) / The caret blink period (as the Windows default)
        constexpr double CARET_PERIOD = 0.53;

        // Мигание каретки фокусированного поля; true — нужна перерисовка / The focused input caret blink; true — a repaint is needed
        bool blinkCaret(Application::State& state, double time)
        {
            const ui::Node* focus = state.router->focused();

            if (focus == nullptr || !ui::isTextField(*focus) || time < state.caretToggle)
                return false;
            state.caretVisible = !state.caretVisible;
            state.caretToggle = time + CARET_PERIOD;
            return true;
        }

        /*
        ** Сколько можно спать: до мигания каретки, появления подсказки, выхода по времени, срока
        ** ближайшего таймера или без предела (отрицательное ожидание — «ждать нечего»).
        **
        ** How long to sleep: until the caret blink, the tooltip appearance, the timed exit, the due time
        ** of the nearest timer or without a limit (a negative wait means "nothing to wait for").
        */
        double sleepTime(const Application::State& state, double time, double exitAfter)
        {
            const ui::Node* focus = state.router->focused();
            double due = state.timers.nextDue();
            double waits[4] = {-1.0, app::tooltipWait(state, time), -1.0, -1.0};
            double sleep = -1.0;

            if (focus != nullptr && ui::isTextField(*focus))
                waits[0] = std::max(0.0, state.caretToggle - time);
            if (exitAfter > 0.0)
                waits[2] = std::max(0.0, exitAfter - time);
            if (due >= 0.0)
                waits[3] = std::max(0.0, due - time);
            for (double wait : waits)
            {
                if (wait >= 0.0 && (sleep < 0.0 || wait < sleep))
                    sleep = wait;
            }
            return sleep;
        }
    }

    namespace app
    {
        // Размер, DPI, перерисовка — графике; остальное — маршрутизатору ввода / The size, DPI, repaint go to the graphics; the rest to the input router
        bool handleEvent(Application::State& state, const platform::Event& event)
        {
            switch (event.kind)
            {
                case platform::EventKind::CLOSE: state.quit = true; return false;
                case platform::EventKind::RESIZE: state.backend->resize(event.width, event.height); break;
                case platform::EventKind::DPI_CHANGED: break;
                case platform::EventKind::EXPOSE: return true;
                default:
                    if (event.kind == platform::EventKind::MOUSE_DOWN || event.kind == platform::EventKind::KEY_DOWN
                        || event.kind == platform::EventKind::MOUSE_WHEEL)
                        app::hideTooltip(state, true);
                    state.caretVisible = true;
                    state.caretToggle = now() + CARET_PERIOD;
                    return state.router->handle(event);
            }
            ui::markDirty(*state.document.root, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
            return true;
        }
    }

    /*
    ** Цикл: все события → кадр, если что-то изменилось или идёт анимация (vsync задаёт темп),
    ** иначе сон до события ОС или до мигания каретки. Окно показывается после первого кадра.
    ** Задачи других потоков (post) выполняются после событий: сообщение-будильник приходит как событие.
    ** Таймеры — после задач; сон длится не дольше, чем до срока ближайшего таймера.
    **
    ** The loop: all events → a frame if something changed or an animation runs (vsync sets the pace),
    ** otherwise sleeping until an OS event or the caret blink. The window is shown after the first frame.
    ** The tasks of other threads (post) run after the events: the wake-up message arrives as an event.
    ** The timers run after the tasks; the sleep lasts no longer than until the nearest timer due time.
    */
    int Application::run()
    {
        State& state = *_state;
        platform::Event event;

        if (!state.document.root)
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::APP_NO_MARKUP));
        app::createGraphics(state);
        state.window->setLiveResize(LiveResize{&state});
        app::renderFrame(state);
        state.window->show();
        WakeScope wake(state);
        while (!state.quit)
        {
            bool changed = false;
            while (state.window->pollEvent(event))
                changed = app::handleEvent(state, event) || changed;
            changed = app::runTasks(state) || changed;
            changed = state.timers.runDue(app::now()) || changed;
            changed = app::deliverWebMessages(state) || changed;
            changed = blinkCaret(state, app::now()) || changed;
            changed = app::updateTooltip(state, app::now()) || changed;
            if (state.options.exitAfter > 0.0 && app::now() >= state.options.exitAfter)
                state.quit = true;
            if (changed || state.animating)
                app::renderFrame(state);
            else
                state.window->waitEvents(sleepTime(state, app::now(), state.options.exitAfter));
        }
        return 0;
    }
}
