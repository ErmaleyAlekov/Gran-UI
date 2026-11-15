#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

@implementation GranCocoaNativeWindow

/*
** Единая точка ввода: клавиши и мышь сначала видит CocoaWindow (cocoa_view_input.mm, cocoa_mouse.mm). Клавиши
** поглощаются всегда; мышь — только при перетаскивании своей рамки (его уже начал AppKit), остальное идёт AppKit
** дальше (кнопки заголовка, края окна).
**
** The single input point: CocoaWindow sees keys and the mouse first (cocoa_view_input.mm, cocoa_mouse.mm). Keys are
** always consumed; the mouse — only when dragging the custom frame (AppKit has already started it), the rest goes on
** to AppKit (the title buttons, the window edges).
*/
- (void)sendEvent:(NSEvent*)event
{
    gran::platform::cocoa::CocoaWindow* owner = self.owner;

    if (owner != nullptr && (owner->key(event) || owner->mouse(event)))
        return;
    [super sendEvent:event];
}

/*
** Сочетания с Cmd и Control AppKit сначала предлагает «эквивалентам клавиш»: окну (оно перебрало бы поля по
** Control+Tab) и меню. Пункт меню программы (Cmd+Q) выполняется как обычно, любое другое сочетание уходит движку
** ровно один раз — до sendEvent: оно уже не дойдёт.
**
** AppKit first offers chords with Cmd and Control to the "key equivalents": the window (it would cycle the fields on
** Control+Tab) and the menu. A program menu item (Cmd+Q) runs as usual, any other chord goes to the engine exactly
** once — it does not reach sendEvent: after that.
*/
- (BOOL)performKeyEquivalent:(NSEvent*)event
{
    gran::platform::cocoa::CocoaWindow* owner = self.owner;
    NSEventModifierFlags chord = NSEventModifierFlagCommand | NSEventModifierFlagControl;

    if (owner == nullptr || event.type != NSEventTypeKeyDown || (event.modifierFlags & chord) == 0)
        return [super performKeyEquivalent:event];
    if ([NSApp.mainMenu performKeyEquivalent:event])
        return YES;
    owner->key(event);
    return YES;
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
