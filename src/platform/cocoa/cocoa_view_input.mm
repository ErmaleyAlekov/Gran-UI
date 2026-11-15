#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

@implementation GranCocoaView (Input)

/*
** Команды правки, которые метод ввода выводит из клавиш (insertNewline:, moveLeft:, deleteBackward:), не нужны:
** их смысл уже выбрало поле ввода движка по KEY_DOWN. Базовый метод не вызывается — он подал бы звуковой сигнал.
**
** The editing commands the input method derives from keys (insertNewline:, moveLeft:, deleteBackward:) are not needed:
** the engine input field has already chosen their meaning by KEY_DOWN. The base method is not called — it would beep.
*/
- (void)doCommandBySelector:(SEL)selector
{
    (void)selector;
}

// Щелчок по неактивному окну доходит до программы (как в Windows и X11) / A click on an inactive window reaches the program (as on Windows and X11)
- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
    (void)event;
    return YES;
}

@end

namespace gran::platform::cocoa
{
    /*
    ** Клавиатура окна — в одном месте, как мышь (sendEvent: и performKeyEquivalent: окна): нажатие — KEY_DOWN движку,
    ** затем методу ввода (interpretKeyEvents → insertText: даёт CHARACTER). Цепочка ответчиков AppKit клавишу не видит:
    ** иначе Control+Tab забрал бы перебор полей AppKit, а Tab и Esc — свои действия окна. Пока метод ввода набирает
    ** строку (IME), клавиши принадлежат ему: Enter или стрелка выбирают кандидата, а не уходят программе.
    **
    ** The window keyboard is in one place, as the mouse (the window sendEvent: and performKeyEquivalent:): a press is
    ** KEY_DOWN to the engine, then to the input method (interpretKeyEvents → insertText: gives CHARACTER). The AppKit
    ** responder chain does not see the key: otherwise the AppKit field cycling would take Control+Tab, and Tab and Esc —
    ** the own window actions. While the input method composes a string (IME), the keys belong to it: Enter or an arrow
    ** choose a candidate instead of going to the program.
    */
    bool CocoaWindow::key(NSEvent* native) noexcept
    {
        NSEventType type = native.type;

        if (type != NSEventTypeKeyDown && type != NSEventTypeKeyUp)
            return false;
        if (_view.hasMarkedText)
        {
            if (type == NSEventTypeKeyDown)
                [_view interpretKeyEvents:@[native]];
            return true;
        }
        if (type == NSEventTypeKeyUp)
            keyUp(native);
        else if (keyDown(native))
            [_view interpretKeyEvents:@[native]];
        return true;
    }

    // Якорь компоновки категории Input (cocoa_window_impl.hpp) / The Input category link anchor (cocoa_window_impl.hpp)
    void linkViewInput() noexcept
    {
    }
}
