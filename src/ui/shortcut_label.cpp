#include "gran/ui/command.hpp"
#include "gran/platform/host.hpp"

namespace gran::ui
{
    namespace
    {
        // Короткое имя клавиши для подписи (как в меню Windows); прочие — keyName / The short key name for a label (as in the Windows menus); the others are keyName
        const char* labelName(platform::Key key) noexcept
        {
            switch (key)
            {
                case platform::Key::ESCAPE: return "Esc";
                case platform::Key::DELETE_KEY: return "Del";
                case platform::Key::INSERT: return "Ins";
                case platform::Key::LEFT: return "Left";
                case platform::Key::RIGHT: return "Right";
                case platform::Key::UP: return "Up";
                case platform::Key::DOWN: return "Down";
                case platform::Key::PAGE_UP: return "PgUp";
                case platform::Key::PAGE_DOWN: return "PgDn";
                default: return platform::keyName(key);
            }
        }
    }

    // Вид подписи — по ОС сборки: знаки на macOS, текст в остальных / The label look is by the build OS: signs on macOS, text on the others
    std::string shortcutText(Shortcut shortcut)
    {
        if (platform::hostOs() == platform::HostOs::MACOS)
            return shortcutText(shortcut, ShortcutStyle::SYMBOLS);
        return shortcutText(shortcut, ShortcutStyle::TEXT);
    }

    // Порядок модификаторов — Ctrl, Shift, Alt (как в меню Windows: Ctrl+Shift+S) / The modifier order is Ctrl, Shift, Alt (as in the Windows menus: Ctrl+Shift+S)
    std::string shortcutText(Shortcut shortcut, ShortcutStyle style)
    {
        std::string text;

        if (style == ShortcutStyle::SYMBOLS)
            return shortcutSymbols(shortcut);

        if ((shortcut.modifiers & platform::MODIFIER_CONTROL) != 0)
            text += "Ctrl+";
        if ((shortcut.modifiers & platform::MODIFIER_SHIFT) != 0)
            text += "Shift+";
        if ((shortcut.modifiers & platform::MODIFIER_ALT) != 0)
            text += "Alt+";
        text += labelName(shortcut.key);
        return text;
    }

    // Модификаторы сравниваются целиком: Ctrl+S не срабатывает на Ctrl+Shift+S / The modifiers are compared entirely: Ctrl+S does not fire on Ctrl+Shift+S
    bool sameShortcut(Shortcut left, Shortcut right) noexcept
    {
        return left.key == right.key && left.modifiers == right.modifiers;
    }
}
