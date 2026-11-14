#include "gran/platform/headless/headless_window.hpp"

namespace gran::platform
{
    // Обработчик хранится: ОС спросила бы его о каждой точке рамки, тест — через frameRole
    // The handler is kept: the OS would ask it about every frame point, a test — via frameRole
    void HeadlessWindow::setFrameHitTest(std::function<WindowRole(Vec2)> callback)
    {
        _frameHitTest = std::move(callback);
    }

    WindowRole HeadlessWindow::frameRole(Vec2 point) const
    {
        if (!_frameHitTest)
            return WindowRole::NONE;
        return _frameHitTest(point);
    }

    void HeadlessWindow::liveResize()
    {
        if (_liveResize)
            _liveResize();
    }
}
