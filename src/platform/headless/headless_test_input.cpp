#include "gran/platform/headless/headless_window.hpp"

namespace gran::platform
{
    // RESIZE несёт пиксели устройства, как у окна ОС / RESIZE carries device pixels, as an OS window does
    void HeadlessWindow::resizeTo(float width, float height)
    {
        Event event;

        _width = width;
        _height = height;
        Size pixels = pixelSize();
        event.kind = EventKind::RESIZE;
        event.width = static_cast<std::uint32_t>(pixels.width);
        event.height = static_cast<std::uint32_t>(pixels.height);
        event.scale = _scale;
        pushEvent(event);
    }

    // Как Win32 WM_DPICHANGED: сначала новый масштаб, затем размер в новых пикселях
    // As Win32 WM_DPICHANGED: first the new scale, then the size in new pixels
    void HeadlessWindow::changeScale(float scale)
    {
        Event event;

        _scale = scale;
        event.kind = EventKind::DPI_CHANGED;
        event.scale = scale;
        pushEvent(event);
        resizeTo(_width, _height);
    }

    // Пути кладутся до события DROP: обработчик броска заберёт их через takeDroppedFiles
    // The paths are put before the DROP event: the drop handler takes them via takeDroppedFiles
    void HeadlessWindow::dropFiles(Vec2 position, const std::vector<std::string>& paths)
    {
        Event event;

        event.position = position;
        event.kind = EventKind::DRAG_MOVE;
        pushEvent(event);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _drops.push_back(paths);
        }
        event.kind = EventKind::DROP;
        pushEvent(event);
    }
}
