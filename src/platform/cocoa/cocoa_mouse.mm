#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

#include <algorithm>

namespace gran::platform::cocoa
{
    namespace
    {
        // Вид события мыши движка по типу NSEvent; false — это не событие мыши / The engine mouse event kind by the NSEvent type; false — not a mouse event
        bool mouseKind(NSEventType type, EventKind& kind) noexcept
        {
            switch (type)
            {
                case NSEventTypeLeftMouseDown: case NSEventTypeRightMouseDown: case NSEventTypeOtherMouseDown:
                    kind = EventKind::MOUSE_DOWN; return true;
                case NSEventTypeLeftMouseUp: case NSEventTypeRightMouseUp: case NSEventTypeOtherMouseUp:
                    kind = EventKind::MOUSE_UP; return true;
                case NSEventTypeMouseMoved: case NSEventTypeLeftMouseDragged: case NSEventTypeRightMouseDragged:
                case NSEventTypeOtherMouseDragged:
                    kind = EventKind::MOUSE_MOVE; return true;
                case NSEventTypeScrollWheel:
                    kind = EventKind::MOUSE_WHEEL; return true;
                case NSEventTypeMouseExited:
                    kind = EventKind::MOUSE_LEAVE; return true;
                default:
                    return false;
            }
        }
    }

    /*
    ** Все события мыши окна проходят здесь раньше видов (sendEvent: окна). Нажатие вне вида (системный заголовок)
    ** остаётся AppKit; нажатие на своей рамке может стать перетаскиванием окна. Перемещения и отпускание после
    ** нажатия AppKit присылает этому окну и за его краем — это и есть захват мыши до отпускания.
    **
    ** All the window mouse events pass here before the views (the window sendEvent:). A press outside the view (the
    ** system title) stays with AppKit; a press on the custom frame may become a window drag. AppKit sends the moves and
    ** the release after a press to this window even beyond its edge — that is the mouse capture until the release.
    */
    bool CocoaWindow::mouse(NSEvent* native) noexcept
    {
        Event event;

        if (!mouseKind(native.type, event.kind))
            return false;
        event.position = point(native);
        event.modifiers = modifiersFromFlags(native.modifierFlags);
        if (event.kind == EventKind::MOUSE_LEAVE && native.trackingArea.owner != _view)
            return false;
        if (event.kind == EventKind::MOUSE_WHEEL)
        {
            wheel(native, event.position);
            return false;
        }
        // Кнопки «назад» и «вперёд» (3, 4 и далее) движок не различает / The engine does not tell the "back" and "forward" buttons (3, 4 and on)
        if ((native.type == NSEventTypeOtherMouseDown || native.type == NSEventTypeOtherMouseUp) && native.buttonNumber != 2)
            return false;
        bool inside = NSPointInRect(NSMakePoint(event.position.x, event.position.y), _view.bounds);
        if (event.kind == EventKind::MOUSE_DOWN && !inside)
            return false;
        if (event.kind == EventKind::MOUSE_DOWN && frameMouse(native, event.position))
            return true;
        button(native, event);
        post(event);
        return false;
    }

    // Точка события в пунктах вида (вид перевёрнут: начало — левый верхний угол) / The event point in view points (the view is flipped: the origin is the top left corner)
    Vec2 CocoaWindow::point(NSEvent* native) const noexcept
    {
        NSPoint local = [_view convertPoint:native.locationInWindow fromView:nil];

        return {static_cast<float>(local.x), static_cast<float>(local.y)};
    }

    /*
    ** Кнопка и число щелчков (счёт ОС — clickCount, по настройке двойного щелчка). Control+щелчок левой — правая
    ** кнопка без META (соглашение macOS, контекстное меню); её отпускание тоже правое.
    **
    ** The button and the click count (the OS count — clickCount, by the double click setting). Control+left click is the
    ** right button without META (the macOS convention, the context menu); its release is right too.
    */
    void CocoaWindow::button(NSEvent* native, Event& event) noexcept
    {
        NSEventType type = native.type;

        if (type == NSEventTypeRightMouseDown || type == NSEventTypeRightMouseUp || type == NSEventTypeRightMouseDragged)
            event.button = MouseButton::RIGHT;
        else if (type == NSEventTypeOtherMouseDown || type == NSEventTypeOtherMouseUp)
            event.button = MouseButton::MIDDLE;
        if (type == NSEventTypeLeftMouseDown)
            _controlClick = (event.modifiers & MODIFIER_META) != 0;
        if (_controlClick && event.button == MouseButton::LEFT && event.kind != EventKind::MOUSE_MOVE)
        {
            event.button = MouseButton::RIGHT;
            event.modifiers = static_cast<std::uint8_t>(event.modifiers & ~MODIFIER_META);
        }
        if (type == NSEventTypeLeftMouseUp)
            _controlClick = false;
        if (event.kind == EventKind::MOUSE_DOWN)
            event.clicks = static_cast<std::uint8_t>(std::min<NSInteger>(255, std::max<NSInteger>(1, native.clickCount)));
    }

    /*
    ** Своя рамка (TitleBar::CUSTOM): нажатие на участке роли DRAG перетаскивает окно средствами AppKit,
    ** двойной щелчок — как в заголовке macOS (по настройке «Двойное нажатие на заголовок окна»: развернуть,
    ** свернуть или ничего). Края окна своей рамки AppKit растягивает сам.
    **
    ** The custom frame (TitleBar::CUSTOM): a press on a DRAG role area drags the window by AppKit means, a double click is
    ** as in the macOS title (by the "Double-click a window's title bar" setting: zoom, minimize or nothing). AppKit
    ** resizes the custom frame window edges itself.
    */
    bool CocoaWindow::frameMouse(NSEvent* native, Vec2 position) noexcept
    {
        WindowRole role = WindowRole::NONE;

        if (native.type != NSEventTypeLeftMouseDown || !_custom || !_frameHit)
            return false;
        try { role = _frameHit(position); } catch (...) { return false; }
        if (role != WindowRole::DRAG)
            return false;
        if (native.clickCount < 2)
        {
            [_window performWindowDragWithEvent:native];
            return true;
        }
        NSString* action = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleActionOnDoubleClick"];
        if ([action isEqualToString:@"Minimize"])
            [_window miniaturize:nil];
        else if (![action isEqualToString:@"None"])
            [_window zoom:nil];
        return true;
    }
}
