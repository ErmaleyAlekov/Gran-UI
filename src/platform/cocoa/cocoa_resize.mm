#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

namespace gran::platform::cocoa
{
    // Событие в очередь; std::bad_alloc не должен уйти в кадры AppKit — событие теряется
    // An event into the queue; std::bad_alloc must not go into AppKit frames — the event is lost
    void CocoaWindow::post(const Event& event) noexcept
    {
        try
        {
            _events.push_back(event);
        }
        catch (...)
        {
            // Нехватка памяти: одно событие ввода потеряно, программа продолжает работу
            // Out of memory: one input event is lost, the program keeps working
        }
    }

    void CocoaWindow::push(EventKind kind) noexcept
    {
        Event event;

        event.kind = kind;
        post(event);
    }

    /*
    ** Вид сменил размер: RESIZE в пикселях устройства (только если они изменились). Пока край окна тянут мышью,
    ** AppKit крутит свой цикл и до waitEvents программа не дойдёт — кадр рисуется здесь же через обратный вызов
    ** setLiveResize, обёрнутый в noexcept-барьер (контракт платформы).
    **
    ** The view changed its size: RESIZE in device pixels (only if they changed). While the window edge is dragged
    ** with the mouse, AppKit spins its own loop and the program does not reach waitEvents — the frame is drawn right
    ** here through the setLiveResize callback wrapped into a noexcept barrier (the platform contract).
    */
    void CocoaWindow::resized() noexcept
    {
        Size pixels = pixelSize();

        if (pixels.width != _pixels.width || pixels.height != _pixels.height)
        {
            Event event;
            _pixels = pixels;
            event.kind = EventKind::RESIZE;
            event.width = static_cast<std::uint32_t>(pixels.width);
            event.height = static_cast<std::uint32_t>(pixels.height);
            post(event);
        }
        if (!_liveResize || !_view.inLiveResize)
            return;
        try
        {
            _liveResize();
        }
        catch (...)
        {
            // Ошибку кадра уже записал в журнал сам обратный вызов; дальше AppKit её не пускаем
            // The callback itself has already logged the frame error; it is not let further into AppKit
        }
    }

    /*
    ** Окно перешло на монитор с другим масштабом (Retina ↔ обычный): слой получает новый масштаб, иначе
    ** AppKit растянет кадр; затем DPI_CHANGED и RESIZE (пикселей стало больше или меньше при тех же пунктах).
    **
    ** The window moved to a monitor with another scale (Retina ↔ regular): the layer gets the new scale, otherwise
    ** AppKit stretches the frame; then DPI_CHANGED and RESIZE (there are more or fewer pixels for the same points).
    */
    void CocoaWindow::backingChanged() noexcept
    {
        Event event;

        _layer.contentsScale = _window.backingScaleFactor;
        event.kind = EventKind::DPI_CHANGED;
        event.scale = scale();
        post(event);
        resized();
    }

    // Обратный вызов перерисовки во время изменения размера мышью / The repaint callback while resizing with the mouse
    void CocoaWindow::setLiveResize(std::function<void()> callback)
    {
        _liveResize = std::move(callback);
    }
}
