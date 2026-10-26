#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    namespace
    {
        // Виртуальная клавиша → Key / A virtual key → Key
        Key mapKey(WPARAM virtualKey) noexcept
        {
            static const struct { WPARAM code; Key key; } KEYS[] = {
                {VK_BACK, Key::BACKSPACE}, {VK_TAB, Key::TAB}, {VK_RETURN, Key::ENTER}, {VK_ESCAPE, Key::ESCAPE},
                {VK_SPACE, Key::SPACE}, {VK_LEFT, Key::LEFT}, {VK_RIGHT, Key::RIGHT}, {VK_UP, Key::UP},
                {VK_DOWN, Key::DOWN}, {VK_HOME, Key::HOME}, {VK_END, Key::END}, {VK_DELETE, Key::DELETE_KEY},
                {VK_PRIOR, Key::PAGE_UP}, {VK_NEXT, Key::PAGE_DOWN}, {'A', Key::A}, {'C', Key::C}, {'V', Key::V},
                {'X', Key::X}, {'Z', Key::Z}};

            for (const auto& entry : KEYS)
            {
                if (entry.code == virtualKey)
                    return entry.key;
            }
            return Key::UNKNOWN;
        }

        // Символ: суррогатная пара UTF-16 склеивается, управляющие символы отбрасываются
        // A character: a UTF-16 surrogate pair is joined, control characters are dropped
        void translateCharacter(Win32Window& window, WPARAM wParam) noexcept
        {
            char32_t unit = static_cast<char32_t>(wParam);
            Event event;

            if (unit >= 0xD800 && unit <= 0xDBFF)
            {
                window.highSurrogate = static_cast<char16_t>(unit);
                return;
            }
            if (unit >= 0xDC00 && unit <= 0xDFFF && window.highSurrogate != 0)
                unit = 0x10000 + ((static_cast<char32_t>(window.highSurrogate) - 0xD800) << 10) + (unit - 0xDC00);
            window.highSurrogate = 0;
            if (unit < 0x20 || unit == 0x7F)
                return;
            event.kind = EventKind::CHARACTER;
            event.character = unit;
            window.push(event);
        }
    }

    // Нажатые модификаторы / The pressed modifiers
    std::uint8_t currentModifiers() noexcept
    {
        std::uint8_t modifiers = 0;

        if (GetKeyState(VK_SHIFT) < 0)
            modifiers |= MODIFIER_SHIFT;
        if (GetKeyState(VK_CONTROL) < 0)
            modifiers |= MODIFIER_CONTROL;
        if (GetKeyState(VK_MENU) < 0)
            modifiers |= MODIFIER_ALT;
        return modifiers;
    }

    // Клавиши (WM_SYSKEYDOWN уходит и в DefWindowProc: Alt+F4 работает) и символы
    // Keys (WM_SYSKEYDOWN also goes to DefWindowProc: Alt+F4 works) and characters
    bool translateKeyboard(Win32Window& window, UINT message, WPARAM wParam)
    {
        Event event;

        if (message == WM_CHAR)
        {
            translateCharacter(window, wParam);
            return true;
        }
        if (message != WM_KEYDOWN && message != WM_KEYUP && message != WM_SYSKEYDOWN)
            return false;
        event.kind = EventKind::KEY_DOWN;
        if (message == WM_KEYUP)
            event.kind = EventKind::KEY_UP;
        event.key = mapKey(wParam);
        event.modifiers = currentModifiers();
        window.push(event);
        return message != WM_SYSKEYDOWN;
    }
}
