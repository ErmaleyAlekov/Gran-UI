#include "gran/app/internal/app_state.hpp"

namespace gran
{
    // До создания окна ОС главное окно не развёрнуто.
    // Before the OS window is created the main window is not maximized.
    bool Application::isMaximized() const
    {
        if (_state->windows.empty() || !_state->windows.front()->window)
            return false;
        return _state->windows.front()->window->isMaximized();
    }
}

namespace gran::app
{
    namespace
    {
        // Окно живёт дольше своего HWND; новый маршрутизатор подхватывается после reload.
        // The window outlives its HWND; a new router is picked up after reload.
        struct FramePicker
        {
            AppWindow* win;

            platform::WindowRole operator()(Vec2 point) const
            {
                if (win->shared.reloading || !win->router)
                    return platform::WindowRole::NONE;
                return win->router->windowRoleAt(point);
            }
        };
    }

    // Подключение только после создания маршрутизатора.
    // Connect only after the router has been created.
    void connectWindowFrame(AppWindow& win)
    {
        win.window->setFrameHitTest(FramePicker{&win});
    }

    // Максимизация через ОС обновляет состояния перед каскадом стилей этого же кадра.
    // Maximization through the OS updates states before cascading styles in this same frame.
    void syncWindowFrame(AppWindow& win)
    {
        ui::setWindowMaximized(*win.document.root, win.window->isMaximized());
    }
}
