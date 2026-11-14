#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    /*
    ** Цикл до закрытия главного окна: окна получают графику и первый кадр, затем шаги цикла со сном
    ** в простое (application_run.cpp). Тот же шаг без сна делает AppDriver в тестах — цикл один.
    **
    ** The loop until the main window closes: the windows get their graphics and the first frame, then loop steps with sleeping
    ** when idle (application_run.cpp). AppDriver does the same step without sleeping in tests — the loop is one.
    */
    int Application::run()
    {
        State& state = *_state;

        if (state.windows.empty())
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::APP_NO_MARKUP));
        app::openWindows(state);
        app::RunScope scope(state);
        while (!state.quit && app::runLoopStep(state, true) != app::LoopStep::QUIT)
        {
        }
        app::saveSettings(state);
        return 0;
    }
}
