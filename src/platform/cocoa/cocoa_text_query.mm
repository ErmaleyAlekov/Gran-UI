#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

/*
** Вопросы метода ввода о тексте вида. Текст поля живёт в движке, а не в виде, поэтому ответы минимальные:
** выделение — только внутри набираемой строки, подстрок и индексов по точке нет. Окно кандидатов
** показывается у левого верхнего угла вида, пока в контракте нет Window::setTextInputArea (пункт 2.5).
**
** The input method questions about the view text. The field text lives in the engine, not in the view, so the
** answers are minimal: the selection is only inside the composed string, there are no substrings and no indices by
** point. The candidate window is shown at the top left corner of the view until the contract has
** Window::setTextInputArea (item 2.5).
*/
@implementation GranCocoaView (TextQuery)

- (NSRange)selectedRange
{
    if (self.marked.length == 0)
        return NSMakeRange(NSNotFound, 0);
    return self.markedSelection;
}

// Набираемая строка без оформления (подчёркивание рисовал бы движок) / The composed string without styling (the engine would draw the underline)
- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText
{
    return @[];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actual
{
    (void)range;
    (void)actual;
    return nil;
}

// Место окна кандидатов в координатах экрана / The candidate window place in screen coordinates
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actual
{
    NSRect local = NSMakeRect(0, 0, 1, 20);

    (void)range;
    if (actual != nullptr)
        *actual = NSMakeRange(NSNotFound, 0);
    if (self.window == nil)
        return NSZeroRect;
    return [self.window convertRectToScreen:[self convertRect:local toView:nil]];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    (void)point;
    return NSNotFound;
}

@end
