#include "gran/platform/window.hpp"

namespace gran::platform
{
    // Платформы без своей рамки не запрашивают роли дерева.
    // Platforms without a custom frame do not query tree roles.
    void Window::setFrameHitTest(std::function<WindowRole(Vec2)>) {}
    void* Window::nativeDisplay() const { return nullptr; }
}
