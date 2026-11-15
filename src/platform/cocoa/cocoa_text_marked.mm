#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

/*
** Набираемая методом ввода строка (IME). Своего текста у вида нет: набираемая строка занимает весь его «документ»,
** выделение — только внутри неё (по контракту IME — минимум, событие COMPOSITION появится в пункте 2.5).
**
** The string composed by an input method (IME). The view has no own text: the composed string takes its whole
** "document", the selection is only inside it (the IME contract is the minimum, the COMPOSITION event comes in item 2.5).
*/
@implementation GranCocoaView (TextMarked)

- (BOOL)hasMarkedText
{
    return self.marked.length != 0;
}

- (NSRange)markedRange
{
    if (self.marked.length == 0)
        return NSMakeRange(NSNotFound, 0);
    return NSMakeRange(0, self.marked.length);
}

- (NSRange)selectedRange
{
    if (self.marked.length == 0)
        return NSMakeRange(NSNotFound, 0);
    return self.markedSelection;
}

@end

namespace gran::platform::cocoa
{
    // Якорь компоновки категории TextMarked (cocoa_window_impl.hpp) / The TextMarked category link anchor (cocoa_window_impl.hpp)
    void linkTextMarked() noexcept
    {
    }
}
