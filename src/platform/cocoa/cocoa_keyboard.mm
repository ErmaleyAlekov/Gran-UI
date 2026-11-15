#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"
#include "gran/utils/utf8.hpp"

#include <string_view>

namespace gran::platform::cocoa
{
    namespace
    {
        /*
        ** Набрала ли клавиша с Option символ (Option+E — «´», Option+2 — «™»): тогда это набор текста, а не сочетание,
        ** и KEY_DOWN идёт без MODIFIER_ALT (контракт платформы). Служебные клавиши дают символы из личной области
        ** Unicode (0xF700–0xF8FF) или управляющие — они символом не считаются.
        **
        ** Whether the key with Option typed a symbol (Option+E — "´", Option+2 — "™"): then it is typing, not a chord,
        ** and KEY_DOWN goes without MODIFIER_ALT (the platform contract). The service keys give symbols from the Unicode
        ** private area (0xF700–0xF8FF) or control ones — they do not count as symbols.
        */
        bool optionTyped(NSEvent* event) noexcept
        {
            NSString* typed = event.characters;

            if ((event.modifierFlags & NSEventModifierFlagOption) == 0 || typed.length == 0)
                return false;
            unichar first = [typed characterAtIndex:0];
            if (first < 0x20 || first == 0x7F || (first >= 0xF700 && first <= 0xF8FF))
                return false;
            return ![typed isEqualToString:event.charactersIgnoringModifiers];
        }
    }

    // Cmd — «Mod» (MODIFIER_CONTROL), Option — ALT, физический Control — META (контракт платформы, M1)
    // Cmd is "Mod" (MODIFIER_CONTROL), Option is ALT, the physical Control is META (the platform contract, M1)
    std::uint8_t modifiersFromFlags(NSEventModifierFlags flags) noexcept
    {
        std::uint8_t result = 0;

        if ((flags & NSEventModifierFlagShift) != 0)
            result |= MODIFIER_SHIFT;
        if ((flags & NSEventModifierFlagCommand) != 0)
            result |= MODIFIER_CONTROL;
        if ((flags & NSEventModifierFlagOption) != 0)
            result |= MODIFIER_ALT;
        if ((flags & NSEventModifierFlagControl) != 0)
            result |= MODIFIER_META;
        return result;
    }

    /*
    ** KEY_DOWN (автоповтор — тот же KEY_DOWN). Control+Tab становится Mod+Tab: Cmd+Tab забрала система, а вкладкам
    ** документов нужно сочетание. Возврат — передавать ли клавишу методу ввода: с Mod и с Control символов нет.
    **
    ** KEY_DOWN (the autorepeat is the same KEY_DOWN). Control+Tab becomes Mod+Tab: the system took Cmd+Tab, and the
    ** document tabs need a chord. The result is whether to pass the key to the input method: no symbols with Mod or Control.
    */
    bool CocoaWindow::keyDown(NSEvent* native) noexcept
    {
        Event event;

        event.kind = EventKind::KEY_DOWN;
        event.key = keyFromCode(native.keyCode);
        event.modifiers = modifiersFromFlags(native.modifierFlags);
        if (event.key == Key::TAB && (event.modifiers & MODIFIER_META) != 0)
            event.modifiers = static_cast<std::uint8_t>((event.modifiers & ~MODIFIER_META) | MODIFIER_CONTROL);
        if (optionTyped(native))
            event.modifiers = static_cast<std::uint8_t>(event.modifiers & ~MODIFIER_ALT);
        post(event);
        return (event.modifiers & (MODIFIER_CONTROL | MODIFIER_META)) == 0;
    }

    // Отпускание; с нажатым Cmd macOS отпускания не присылает — движок этого и не ждёт (повтор — KEY_DOWN)
    // A release; with Cmd held macOS sends no release — the engine does not expect it either (a repeat is KEY_DOWN)
    void CocoaWindow::keyUp(NSEvent* native) noexcept
    {
        Event event;

        event.kind = EventKind::KEY_UP;
        event.key = keyFromCode(native.keyCode);
        event.modifiers = modifiersFromFlags(native.modifierFlags);
        post(event);
    }

    // Текст метода ввода → CHARACTER по символу UTF-32; Shift и Option уже потрачены на набор и не передаются
    // The input method text → CHARACTER per UTF-32 symbol; Shift and Option are already spent on typing and are not passed
    void CocoaWindow::insertText(NSString* text) noexcept
    {
        const char* bytes = text.UTF8String;
        std::uint8_t modifiers = modifiersFromFlags([NSEvent modifierFlags]);
        std::size_t offset = 0;

        if (bytes == nullptr)
            return;
        std::string_view value(bytes);
        modifiers = static_cast<std::uint8_t>(modifiers & ~(MODIFIER_SHIFT | MODIFIER_ALT));
        while (offset < value.size())
        {
            Event event;
            event.kind = EventKind::CHARACTER;
            event.character = decodeUtf8(value, offset);
            event.modifiers = modifiers;
            if (event.character >= 32 && event.character != 127)
                post(event);
        }
    }
}
