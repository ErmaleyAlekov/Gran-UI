#include "gran/app/internal/app_state.hpp"

namespace gran
{
    namespace
    {
        // Окно ОС главного окна; до run() его ещё нет — команды ничего не делают.
        // The OS window of the main window; before run() it does not exist yet — the commands do nothing.
        platform::Window* mainWindow(const Application::State& state) noexcept
        {
            if (state.windows.empty())
                return nullptr;
            return state.windows.front()->window.get();
        }
    }

    // Свернуть, развернуть или восстановить главное окно (только поток интерфейса).
    // Minimize, maximize or restore the main window (the UI thread only).
    void Application::minimize()
    {
        if (platform::Window* window = mainWindow(*_state))
            window->minimize();
    }

    void Application::maximize()
    {
        if (platform::Window* window = mainWindow(*_state))
            window->maximize();
    }

    void Application::restore()
    {
        if (platform::Window* window = mainWindow(*_state))
            window->restore();
    }

    // Обычное событие закрытия: срабатывает onClose окна, если он задан.
    // The usual close event: the window onClose fires if it is set.
    void Application::close()
    {
        if (platform::Window* window = mainWindow(*_state))
            window->close();
    }
}
