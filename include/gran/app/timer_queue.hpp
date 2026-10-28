#ifndef GRAN_APP_TIMER_QUEUE_HPP
#define GRAN_APP_TIMER_QUEUE_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace gran::app
{
    // Номер таймера; 0 — «нет таймера» / A timer number; 0 — "no timer"
    using TimerId = std::uint64_t;

    /*
    ** Таймер: номер, срок (секунды часов приложения), период (0 — однократный), обработчик.
    ** Обработчик в общем владении: периодический таймер вызывает один и тот же объект (состояние
    ** функтора живёт между срабатываниями), и объект доживает до конца вызова, даже если
    ** обработчик снял свой таймер.
    **
    ** A timer: the number, the due time (seconds of the application clock), the period (0 — one-shot), the handler.
    ** The handler is shared: a periodic timer calls one and the same object (the functor state
    ** lives between firings), and the object lives until the end of the call even if
    ** the handler cleared its own timer.
    */
    struct Timer
    {
        TimerId id = 0;
        double due = 0.0;
        double period = 0.0;
        std::shared_ptr<std::function<void()>> handler;
    };

    /*
    ** Таймеры потока интерфейса (setTimeout / setInterval из браузеров). Очередь не знает часов:
    ** время передаёт вызывающий, поэтому её можно проверить без окна и без ожидания.
    ** Обработчик может ставить и снимать таймеры, в том числе свой: проход видит только таймеры,
    ** поставленные до него, — новый таймер с нулевой задержкой сработает на следующем проходе,
    ** и цикл событий не зависнет. Пропущенные срабатывания периодического таймера (поток был
    ** занят) не догоняются пачкой: следующий срок — через период от текущего времени.
    **
    ** The UI thread timers (setTimeout / setInterval from browsers). The queue knows no clock:
    ** the caller passes the time, so it can be checked without a window and without waiting.
    ** A handler may set and clear timers, its own one too: a pass sees only the timers
    ** set before it — a new timer with a zero delay fires on the next pass,
    ** and the event loop does not hang. Missed firings of a periodic timer (the thread was
    ** busy) are not caught up in a burst: the next due time is one period from the current time.
    */
    class TimerQueue
    {
    public:
        // ---- timer_queue.cpp ----

        // Поставить таймер на срок due; period > 0 — повторять с этим периодом / Set a timer for the due time; period > 0 — repeat with this period
        TimerId add(double due, double period, std::function<void()> handler);

        // Снять таймер; false — такого нет (уже сработал или снят) / Clear a timer; false — there is none (already fired or cleared)
        bool cancel(TimerId id) noexcept;

        // Ближайший срок; отрицательное — таймеров нет / The nearest due time; negative — there are no timers
        double nextDue() const noexcept;

        // Число стоящих таймеров / The number of pending timers
        std::size_t size() const noexcept;

        // ---- timer_run.cpp ----

        // Выполнить таймеры со сроком не позже now (по порядку сроков); true — какой-то сработал / Run the timers due not later than now (in due order); true — some fired
        bool runDue(double now);

    private:
        std::vector<Timer> _timers;
        TimerId _nextId = 1;
    };
}

#endif
