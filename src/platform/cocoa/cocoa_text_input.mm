#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

namespace
{
    // Метод ввода присылает NSString или NSAttributedString — нужен только текст / The input method sends NSString or NSAttributedString — only the text is needed
    NSString* plainText(id text)
    {
        // id неявно приводится к указателю на класс Objective-C / id converts implicitly to an Objective-C class pointer
        NSAttributedString* attributed = text;
        NSString* plain = text;

        if ([text isKindOfClass:[NSAttributedString class]])
            return attributed.string;
        if ([text isKindOfClass:[NSString class]])
            return plain;
        return nil;
    }
}

/*
** Минимум NSTextInputClient по контракту платформы (IME — пункт 2.5): зафиксированный текст становится
** CHARACTER, набираемая строка хранится в виде, но движку пока не показывается (события COMPOSITION ещё нет).
**
** The NSTextInputClient minimum by the platform contract (IME is item 2.5): the committed text becomes CHARACTER,
** the string being composed is kept in the view but not shown to the engine yet (there is no COMPOSITION event yet).
*/
@implementation GranCocoaView (TextInput)

// Фиксация: набираемая строка исчезает, текст уходит движку / Committing: the composed string disappears, the text goes to the engine
- (void)insertText:(id)text replacementRange:(NSRange)range
{
    gran::platform::cocoa::CocoaWindow* owner = self.owner;
    NSString* value = plainText(text);

    (void)range;
    self.marked = nil;
    if (owner != nullptr && value != nil)
        owner->insertText(value);
}

// Метод ввода показывает набираемую строку и выделение в ней / The input method shows the composed string and its selection
- (void)setMarkedText:(id)text selectedRange:(NSRange)selected replacementRange:(NSRange)range
{
    NSString* value = plainText(text);

    (void)range;
    self.marked = nil;
    if (value.length != 0)
        self.marked = value;
    self.markedSelection = selected;
}

- (void)unmarkText
{
    self.marked = nil;
}

- (BOOL)hasMarkedText
{
    return self.marked.length != 0;
}

// Набираемая строка занимает весь «документ» вида: своего текста у вида нет / The composed string takes the whole view "document": the view has no own text
- (NSRange)markedRange
{
    if (self.marked.length == 0)
        return NSMakeRange(NSNotFound, 0);
    return NSMakeRange(0, self.marked.length);
}

@end
