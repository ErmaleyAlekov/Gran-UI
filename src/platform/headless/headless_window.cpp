#include "gran/platform/headless/headless_window.hpp"

#include <cmath>

namespace gran::platform
{
    // Поверхности ОС нет: бэкенд отрисовки создаётся вне экрана / There is no OS surface: the render backend is created offscreen
    void* HeadlessWindow::nativeHandle() const
    {
        return nullptr;
    }

    float HeadlessWindow::scale() const
    {
        return _scale;
    }

    // Логический размер × масштаб, с округлением, как у окна ОС / The logical size × the scale, rounded, as an OS window does
    Size HeadlessWindow::pixelSize() const
    {
        return {std::round(_width * _scale), std::round(_height * _scale)};
    }

    void HeadlessWindow::show()
    {
        _state.visible = true;
    }

    // Обратный вызов хранится: тест вызывает его через liveResize() / The callback is kept: a test calls it via liveResize()
    void HeadlessWindow::setLiveResize(std::function<void()> callback)
    {
        _liveResize = std::move(callback);
    }
}
