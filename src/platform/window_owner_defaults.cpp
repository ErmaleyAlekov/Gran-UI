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

    // Окно без места (тестовые окна): сохранять нечего / A window without a place (test windows): nothing to keep
    bool Window::placement(WindowPlacement&) const
    {
        return false;
    }

    // Окно без места: восстанавливать некуда / A window without a place: nowhere to restore
    void Window::setPlacement(const WindowPlacement&)
    {
    }
}
