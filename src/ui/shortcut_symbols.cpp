#include "gran/ui/command.hpp"

namespace gran::ui
{
    namespace
    {
        // Знак клавиши в меню macOS; прочие — keyName ("A", "F5") / The key sign in macOS menus; the others are keyName ("A", "F5")
        const char* symbolName(platform::Key key) noexcept
        {
            switch (key)
            {
                case platform::Key::ESCAPE: return "⎋";
                case platform::Key::DELETE_KEY: return "⌦";
                case platform::Key::BACKSPACE: return "⌫";
                case platform::Key::ENTER: return "↩";
                case platform::Key::TAB: return "⇥";
                case platform::Key::LEFT: return "←";
                case platform::Key::RIGHT: return "→";
                case platform::Key::UP: return "↑";
                case platform::Key::DOWN: return "↓";
                case platform::Key::PAGE_UP: return "⇞";
                case platform::Key::PAGE_DOWN: return "⇟";
                case platform::Key::HOME: return "↖";
                case platform::Key::END: return "↘";
                default: return platform::keyName(key);
            }
        }
    }

    /*
    ** Порядок знаков — как в меню macOS: ⌃ (физический Control, MODIFIER_META), ⌥ (Option), ⇧ (Shift),
    ** ⌘ (Cmd — это «Mod», MODIFIER_CONTROL), затем клавиша; без «+» между ними.
    **
    ** The sign order is as in macOS menus: ⌃ (the physical Control, MODIFIER_META), ⌥ (Option), ⇧ (Shift),
    ** ⌘ (Cmd — that is "Mod", MODIFIER_CONTROL), then the key; without "+" between them.
    */
    std::string shortcutSymbols(Shortcut shortcut)
    {
        std::string text;

        if ((shortcut.modifiers & platform::MODIFIER_META) != 0)
            text += "⌃";
        if ((shortcut.modifiers & platform::MODIFIER_ALT) != 0)
            text += "⌥";
        if ((shortcut.modifiers & platform::MODIFIER_SHIFT) != 0)
            text += "⇧";
        if ((shortcut.modifiers & platform::MODIFIER_CONTROL) != 0)
            text += "⌘";
        text += symbolName(shortcut.key);
        return text;
    }
}
