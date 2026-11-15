#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

/*
** Делегат окна только переводит уведомления AppKit в события очереди. Каждый метод проверяет owner: после
** разрушения CocoaWindow он обнулён, а AppKit ещё может прислать уведомление.
**
** The window delegate only turns AppKit notifications into queue events. Each method checks owner: after
** CocoaWindow is destroyed it is cleared, and AppKit may still send a notification.
*/
@implementation GranCocoaWindowDelegate

// Кнопка закрытия и Cmd+W: окно не закрывается само — программа получает CLOSE и решает (контракт платформы)
// The close button and Cmd+W: the window does not close itself — the program gets CLOSE and decides (the platform contract)
- (BOOL)windowShouldClose:(NSWindow*)sender
{
    (void)sender;
    if (self.owner != nullptr)
        self.owner->push(gran::platform::EventKind::CLOSE);
    return NO;
}

- (void)windowDidResize:(NSNotification*)notification
{
    (void)notification;
    if (self.owner != nullptr)
        self.owner->resized();
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
    (void)notification;
    if (self.owner != nullptr)
        self.owner->push(gran::platform::EventKind::FOCUS_GAINED);
}

// Потеря фокуса: явного захвата мыши у AppKit нет, отпускать нечего; MOUSE_UP не шлётся (контракт платформы)
// Losing the focus: AppKit has no explicit mouse capture, there is nothing to release; no MOUSE_UP is sent (the platform contract)
- (void)windowDidResignKey:(NSNotification*)notification
{
    (void)notification;
    if (self.owner != nullptr)
        self.owner->push(gran::platform::EventKind::FOCUS_LOST);
}

// Другой масштаб экрана (перенос окна между Retina и обычным монитором) / Another screen scale (moving the window between Retina and a regular monitor)
- (void)windowDidChangeBackingProperties:(NSNotification*)notification
{
    (void)notification;
    if (self.owner != nullptr)
        self.owner->backingChanged();
}

@end
