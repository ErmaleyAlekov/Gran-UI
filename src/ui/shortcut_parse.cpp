#include "gran/ui/command.hpp"

#include <cstring>

namespace gran::ui
{
    namespace
    {
        // Сравнение без учёта регистра латиницы / A comparison ignoring the Latin case
        bool sameName(std::string_view left, const char* right) noexcept
        {
            std::size_t length = std::strlen(right);

            if (left.size() != length)
                return false;
            for (std::size_t index = 0; index < length; ++index)
            {
                char a = left[index];
                char b = right[index];
                if (a >= 'a' && a <= 'z')
                    a = static_cast<char>(a - 'a' + 'A');
                if (b >= 'a' && b <= 'z')
                    b = static_cast<char>(b - 'a' + 'A');
                if (a != b)
                    return false;
            }
            return true;
        }

        /*
        ** Клавиша по имени: имена keyName ("Enter", "ArrowLeft", "F5", "S") и короткие, как в меню
        ** Windows (Esc, Del, Ins, Left, PgUp…). UNKNOWN — нет такой клавиши.
        **
        ** A key by its name: the keyName names ("Enter", "ArrowLeft", "F5", "S") and short ones, as in the Windows
        ** menus (Esc, Del, Ins, Left, PgUp…). UNKNOWN — there is no such key.
        */
        platform::Key keyByName(std::string_view name) noexcept
        {
            static const struct { const char* name; platform::Key key; } SHORT[] = {
                {"Esc", platform::Key::ESCAPE}, {"Del", platform::Key::DELETE_KEY}, {"Ins", platform::Key::INSERT},
                {"Left", platform::Key::LEFT}, {"Right", platform::Key::RIGHT}, {"Up", platform::Key::UP},
                {"Down", platform::Key::DOWN}, {"PgUp", platform::Key::PAGE_UP}, {"PgDn", platform::Key::PAGE_DOWN}};

            for (const auto& alias : SHORT)
            {
                if (sameName(name, alias.name))
                    return alias.key;
            }
            for (unsigned code = 1; code <= static_cast<unsigned>(platform::Key::F12); ++code)
            {
                if (sameName(name, platform::keyName(static_cast<platform::Key>(code))))
                    return static_cast<platform::Key>(code);
            }
            return platform::Key::UNKNOWN;
        }

        // Бит модификатора по имени (Mod — Ctrl); 0 — это не модификатор / The modifier bit by its name (Mod is Ctrl); 0 — not a modifier
        std::uint8_t modifierByName(std::string_view name) noexcept
        {
            if (sameName(name, "Mod") || sameName(name, "Ctrl") || sameName(name, "Control"))
                return platform::MODIFIER_CONTROL;
            if (sameName(name, "Shift"))
                return platform::MODIFIER_SHIFT;
            if (sameName(name, "Alt"))
                return platform::MODIFIER_ALT;
            return 0;
        }
    }

    // Части через «+»: все, кроме последней, — модификаторы; повтор модификатора — ошибка / The parts by "+": all but the last are modifiers; a repeated modifier is an error
    bool parseShortcut(std::string_view text, Shortcut& result) noexcept
    {
        Shortcut shortcut;
        std::size_t start = 0;
        std::size_t plus = text.find('+');

        for (; plus != std::string_view::npos; plus = text.find('+', start))
        {
            std::uint8_t bit = modifierByName(text.substr(start, plus - start));
            if (bit == 0 || (shortcut.modifiers & bit) != 0)
                return false;
            shortcut.modifiers = static_cast<std::uint8_t>(shortcut.modifiers | bit);
            start = plus + 1;
        }
        shortcut.key = keyByName(text.substr(start));
        if (shortcut.key == platform::Key::UNKNOWN)
            return false;
        result = shortcut;
        return true;
    }

    // Пустой список (key="") допустим: у команды просто нет сочетания / An empty list (key="") is allowed: the command simply has no shortcut
    bool parseShortcuts(std::string_view list, std::vector<Shortcut>& result)
    {
        std::size_t start = list.find_first_not_of(" \t\r\n");

        while (start != std::string_view::npos)
        {
            std::size_t end = list.find_first_of(" \t\r\n", start);
            Shortcut shortcut;
            if (!parseShortcut(list.substr(start, end - start), shortcut))
                return false;
            result.push_back(shortcut);
            if (end == std::string_view::npos)
                break;
            start = list.find_first_not_of(" \t\r\n", end);
        }
        return true;
    }
}
