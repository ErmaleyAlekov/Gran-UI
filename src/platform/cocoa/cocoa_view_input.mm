#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

@implementation GranCocoaView (Input)

/*
** Нажатие: KEY_DOWN движку, затем — методу ввода (interpretKeyEvents → insertText: даёт CHARACTER). Пока метод
** ввода набирает строку (IME), клавиши принадлежат ему: Enter или стрелка выбирают кандидата, а не уходят программе.
**
** A press: KEY_DOWN to the engine, then to the input method (interpretKeyEvents → insertText: gives CHARACTER). While
** the input method composes a string (IME), the keys belong to it: Enter or an arrow choose a candidate instead of
** going to the program.
*/
- (void)keyDown:(NSEvent*)event
{
    gran::platform::cocoa::CocoaWindow* owner = self.owner;

    if (owner == nullptr)
        return;
    if (self.hasMarkedText || owner->keyDown(event))
        [self interpretKeyEvents:@[event]];
}

// Отпускание; базовый keyUp: не вызывается — он передал бы событие дальше по цепочке и дал бы сигнал «нельзя»
// A release; the base keyUp: is not called — it would pass the event up the chain and beep
- (void)keyUp:(NSEvent*)event
{
    gran::platform::cocoa::CocoaWindow* owner = self.owner;

    if (owner != nullptr && !self.hasMarkedText)
        owner->keyUp(event);
}

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
    // Якорь компоновки категории Input (cocoa_window_impl.hpp) / The Input category link anchor (cocoa_window_impl.hpp)
    void linkViewInput() noexcept
    {
    }
}
