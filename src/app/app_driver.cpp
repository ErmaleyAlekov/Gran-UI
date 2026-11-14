#include "gran/app/app_driver.hpp"
#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    // Как начало run(): без разметки — та же ошибка, затем окна и их первый кадр
    // As the start of run(): without markup — the same error, then the windows and their first frame
    void AppDriver::start()
    {
        Application::State& current = state();

        if (current.windows.empty())
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::APP_NO_MARKUP));
        app::openWindows(current);
        // Повторный start: старое время снимается до нового, иначе его деструктор снял бы получателя пробуждений
        // A repeated start: the old time is removed before the new one, otherwise its destructor would remove the wake target
        current.driverScope.reset();
        current.driverScope = std::make_unique<app::RunScope>(current);
    }

    bool AppDriver::step()
    {
        Application::State& current = state();

        if (current.quit)
            return false;
        return app::runLoopStep(current, false) != app::LoopStep::QUIT;
    }

    // Обычно хватает двух-трёх шагов: событие → кадр → кадр после отложенных уведомлений
    // Usually two or three steps are enough: an event → a frame → a frame after the deferred notifications
    unsigned AppDriver::settle(unsigned limit)
    {
        Application::State& current = state();
        unsigned steps = 0;

        while (steps < limit && !current.quit)
        {
            ++steps;
            if (app::runLoopStep(current, false) != app::LoopStep::BUSY)
                break;
        }
        return steps;
    }
}
