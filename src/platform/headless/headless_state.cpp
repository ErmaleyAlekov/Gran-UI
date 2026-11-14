#include "gran/platform/headless/headless_window.hpp"

namespace gran::platform
{
    void HeadlessWindow::setCursor(CursorShape shape)
    {
        _state.cursor = shape;
    }

    void HeadlessWindow::setEnabled(bool enabled)
    {
        _state.enabled = enabled;
    }

    // Поднять окно: из свёрнутого оно восстанавливается; счётчик — для проверки в тесте
    // Raising the window: it is restored from minimized; the counter is for a test check
    void HeadlessWindow::activate()
    {
        _state.minimized = false;
        ++_state.activations;
    }

    bool HeadlessWindow::placement(WindowPlacement& placement) const
    {
        placement = _state.placement;
        placement.maximized = _state.maximized;
        return true;
    }

    // Место применяется без проверки мониторов (их нет); размер места становится размером окна
    // The place is applied without the monitor check (there are none); the place size becomes the window size
    void HeadlessWindow::setPlacement(const WindowPlacement& placement)
    {
        _state.placement = placement;
        _state.maximized = placement.maximized;
        if (placement.width > 0 && placement.height > 0)
        {
            _width = static_cast<float>(placement.width);
            _height = static_cast<float>(placement.height);
        }
    }
}
