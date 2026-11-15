#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

@implementation GranCocoaNativeWindow

/*
** Единая точка мыши: событие сначала видит CocoaWindow (cocoa_mouse.mm), затем — AppKit (кнопки заголовка,
** края окна, доставка клавиш виду). Поглощается только перетаскивание своей рамки: его уже начал AppKit.
**
** The single mouse point: CocoaWindow sees the event first (cocoa_mouse.mm), then AppKit (the title buttons, the window
** edges, delivering keys to the view). Only dragging the custom frame is consumed: AppKit has already started it.
*/
- (void)sendEvent:(NSEvent*)event
{
    gran::platform::cocoa::CocoaWindow* owner = self.owner;

    if (owner != nullptr && owner->mouse(event))
        return;
    [super sendEvent:event];
}

// Окно со своей рамкой (заголовок прозрачен) тоже получает клавиатуру и становится главным
// A window with a custom frame (a transparent title) also gets the keyboard and becomes the main one
- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

@end
