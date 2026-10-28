#include "gran/platform/x11/x11_window.hpp"

namespace gran::platform::x11
{
    // Linux AT-SPI не подключается неявно: доступность требует отдельного нативного провайдера.
    // Linux AT-SPI is not linked implicitly: accessibility needs a separate native provider.
    void X11Window::setAccessTree(a11y::AccessTree*) {}
    void X11Window::accessFocusChanged() {}
    void X11Window::accessRefresh() {}
}
