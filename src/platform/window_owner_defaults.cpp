#include "gran/platform/window.hpp"

namespace gran::platform
{
    // Окно без выключения: блокировку ввода ведёт приложение / A window without disabling: the application handles the input blocking
    void Window::setEnabled(bool)
    {
    }

    // Окно без активации (тестовые окна) / A window without activation (test windows)
    void Window::activate()
    {
    }
}
