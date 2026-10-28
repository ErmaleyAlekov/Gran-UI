#include "gran/platform/window.hpp"

namespace gran::platform
{
    // Заглушки сохраняют совместимость тестовых и будущих платформенных окон.
    // Defaults preserve compatibility with test windows and future platform windows.
    void Window::minimize() {}
    void Window::maximize() {}
    void Window::restore() {}
    void Window::close() {}
    bool Window::isMaximized() const { return false; }
}
