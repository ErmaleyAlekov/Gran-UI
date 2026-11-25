#include "gran/platform/headless/headless_window.hpp"
#include "gran/platform/internal/pixel_size.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <cmath>

namespace gran::platform
{
    namespace
    {
        // Событие вычисляется до изменения состояния / Compute the event before changing state
        Event resizeEvent(float width, float height, float scale)
        {
            if (!(width >= 0 && height >= 0 && scale > 0 && std::isfinite(scale)))
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::STYLE_BAD_NUMBER, {std::to_string(width) + " x "
                        + std::to_string(height) + " @ " + std::to_string(scale)}));
            Event event;
            event.kind = EventKind::RESIZE;
            event.width = pixelExtent(std::round(width * scale));
            event.height = pixelExtent(std::round(height * scale));
            event.scale = scale;
            return event;
        }
    }
    // RESIZE несёт пиксели устройства, как у окна ОС / RESIZE carries device pixels, as an OS window does
    void HeadlessWindow::resizeTo(float width, float height)
    {
        const Event event = resizeEvent(width, height, _scale);

        _width = width;
        _height = height;
        pushEvent(event);
    }

    // Как Win32 WM_DPICHANGED: сначала новый масштаб, затем размер в новых пикселях
    // As Win32 WM_DPICHANGED: first the new scale, then the size in new pixels
    void HeadlessWindow::changeScale(float scale)
    {
        const Event resized = resizeEvent(_width, _height, scale);
        Event event;

        _scale = scale;
        event.kind = EventKind::DPI_CHANGED;
        event.scale = scale;
        pushEvent(event);
        pushEvent(resized);
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
