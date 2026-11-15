#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

#include <cmath>

namespace gran::platform::cocoa
{
    namespace
    {
        // Высота главного экрана (начало координат AppKit — его левый нижний угол); 0 — экранов нет
        // The main screen height (the AppKit origin is its bottom left corner); 0 — there are no screens
        CGFloat primaryHeight() noexcept
        {
            NSScreen* primary = [NSScreen screens].firstObject;

            if (primary == nil)
                return 0;
            return primary.frame.size.height;
        }
    }

    /*
    ** Место окна — внешняя рамка в пунктах экрана, ось y сверху вниз от верхнего края главного экрана (как в Windows
    ** и X11): AppKit считает y снизу, перевод делает реализация (контракт платформы).
    **
    ** The window place is the outer frame in screen points, the y axis goes top down from the top edge of the main
    ** screen (as on Windows and X11): AppKit counts y from the bottom, the implementation converts (the platform contract).
    */
    bool CocoaWindow::placement(WindowPlacement& placement) const
    {
        CGFloat top = primaryHeight();
        NSRect frame = _window.frame;

        if (top <= 0)
            return false;
        placement.x = static_cast<int>(std::lround(frame.origin.x));
        placement.y = static_cast<int>(std::lround(top - NSMaxY(frame)));
        placement.width = static_cast<int>(std::lround(frame.size.width));
        placement.height = static_cast<int>(std::lround(frame.size.height));
        placement.maximized = _window.zoomed;
        return true;
    }

    // Сохранённое место применяется, только если рамка задевает видимую часть какого-нибудь экрана
    // The saved place is applied only if the frame touches the visible part of some screen
    void CocoaWindow::setPlacement(const WindowPlacement& placement)
    {
        CGFloat top = primaryHeight();
        bool visible = false;

        if (top <= 0 || placement.width <= 0 || placement.height <= 0)
            return;
        NSRect frame = NSMakeRect(placement.x, top - placement.y - placement.height, placement.width, placement.height);
        for (NSScreen* screen in [NSScreen screens])
            visible = visible || NSIntersectsRect(frame, screen.visibleFrame);
        if (!visible)
            return;
        [_window setFrame:frame display:NO];
        _startMaximized = placement.maximized;
    }

    // Роль участка под мышью для своей рамки (перетаскивание за заголовок) / The area role under the pointer for the custom frame (dragging by the title)
    void CocoaWindow::setFrameHitTest(std::function<WindowRole(Vec2)> callback)
    {
        _frameHit = std::move(callback);
    }
}
