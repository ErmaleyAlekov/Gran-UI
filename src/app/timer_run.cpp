#include "gran/app/timer_queue.hpp"

#include <utility>

namespace gran::app
{
    namespace
    {
        // «Нет такого таймера» для поиска по индексу / "No such timer" for an index search
        constexpr std::size_t NONE = static_cast<std::size_t>(-1);

        // Таймер с самым ранним сроком не позже now среди поставленных до прохода (id < limit)
        // The timer with the earliest due time not later than now among those set before the pass (id < limit)
        std::size_t earliestDue(const std::vector<Timer>& timers, double now, TimerId limit) noexcept
        {
            std::size_t found = NONE;

            for (std::size_t index = 0; index < timers.size(); ++index)
            {
                if (timers[index].due > now || timers[index].id >= limit)
                    continue;
                if (found == NONE || timers[index].due < timers[found].due)
                    found = index;
            }
            return found;
        }

        /*
        ** Ход таймера перед вызовом: однократный убирается (его обработчик переезжает к вызывающему),
        ** периодический переносится на следующий срок позже now (вызывающий получает вторую ссылку
        ** на тот же обработчик: таймер может снять сам себя во время вызова).
        **
        ** The timer turn before the call: a one-shot one is removed (its handler moves to the caller),
        ** a periodic one is moved to the next due time later than now (the caller gets a second reference
        ** to the same handler: the timer may clear itself during the call).
        */
        std::shared_ptr<std::function<void()>> takeTurn(std::vector<Timer>& timers, std::size_t index, double now)
        {
            Timer& timer = timers[index];
            std::shared_ptr<std::function<void()>> handler;

            if (timer.period <= 0.0)
            {
                handler = std::move(timer.handler);
                timers.erase(timers.begin() + static_cast<std::ptrdiff_t>(index));
                return handler;
            }
            timer.due += timer.period;
            if (timer.due <= now)
                timer.due = now + timer.period;
            return timer.handler;
        }
    }

    /*
    ** Проход кончается: каждый выбор либо убирает таймер, либо отодвигает его за now,
    ** а таймеры, поставленные обработчиками, в этот проход не попадают.
    **
    ** The pass ends: every choice either removes a timer or moves it past now,
    ** and the timers set by handlers do not get into this pass.
    */
    bool TimerQueue::runDue(double now)
    {
        TimerId limit = _nextId;
        bool fired = false;

        for (std::size_t index = earliestDue(_timers, now, limit); index != NONE;
             index = earliestDue(_timers, now, limit))
        {
            std::shared_ptr<std::function<void()>> handler = takeTurn(_timers, index, now);
            fired = true;
            if (*handler)
                (*handler)();
        }
        return fired;
    }
}
