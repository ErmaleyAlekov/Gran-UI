#include "gran/app/internal/app_state.hpp"

#include <algorithm>
#include <utility>

namespace gran
{
    namespace
    {
        // Наименьший период: таймер с нулевым периодом занял бы поток интерфейса целиком / The smallest period: a timer with a zero period would take the whole UI thread
        constexpr double MIN_INTERVAL = 0.001;
    }

    // Срок — от часов приложения; отрицательная задержка — как нулевая / The due time is from the application clock; a negative delay is like a zero one
    TimerId Application::setTimeout(double seconds, Task handler)
    {
        return _state->timers.add(app::now() + std::max(0.0, seconds), 0.0, std::move(handler));
    }

    // Первый срок — через период / The first due time is one period later
    TimerId Application::setInterval(double seconds, Task handler)
    {
        double period = std::max(MIN_INTERVAL, seconds);

        return _state->timers.add(app::now() + period, period, std::move(handler));
    }

    // Снятие неизвестного номера не ошибка (таймер мог уже сработать) / Clearing an unknown number is not an error (the timer could have fired already)
    void Application::clearTimer(TimerId id)
    {
        _state->timers.cancel(id);
    }
}
