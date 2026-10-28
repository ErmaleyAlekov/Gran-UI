#include "gran/app/timer_queue.hpp"

#include <utility>

namespace gran::app
{
    // Номера растут и не повторяются: снятие старого номера не заденет новый таймер
    // The numbers grow and never repeat: clearing an old number will not hit a new timer
    TimerId TimerQueue::add(double due, double period, std::function<void()> handler)
    {
        Timer timer;

        timer.id = _nextId++;
        timer.due = due;
        timer.period = 0.0;
        if (period > 0.0)
            timer.period = period;
        timer.handler = std::make_shared<std::function<void()>>(std::move(handler));
        _timers.push_back(std::move(timer));
        return _timers.back().id;
    }

    // Порядок остальных не важен, но сохраняется (проще проверять) / The order of the rest does not matter but is kept (easier to check)
    bool TimerQueue::cancel(TimerId id) noexcept
    {
        for (std::size_t index = 0; index < _timers.size(); ++index)
        {
            if (_timers[index].id == id)
            {
                _timers.erase(_timers.begin() + static_cast<std::ptrdiff_t>(index));
                return true;
            }
        }
        return false;
    }

    // Линейный поиск: таймеров у окна — единицы / A linear search: a window has a few timers
    double TimerQueue::nextDue() const noexcept
    {
        double nearest = -1.0;

        for (const Timer& timer : _timers)
        {
            if (nearest < 0.0 || timer.due < nearest)
                nearest = timer.due;
        }
        return nearest;
    }

    // Число таймеров / The timer count
    std::size_t TimerQueue::size() const noexcept
    {
        return _timers.size();
    }
}
