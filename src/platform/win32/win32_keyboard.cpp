#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    namespace
    {
        /*
        ** Виртуальная клавиша → Key. Коды VK букв и цифр совпадают с ASCII ('A'…'Z', '0'…'9'), F1–F12 идут
        ** подряд — для них сдвиг в ряду Key, для остальных таблица.
        **
        ** A virtual key → Key. The VK codes of the letters and the digits match ASCII ('A'…'Z', '0'…'9'), F1–F12 go
        ** in a row — they are an offset in the Key row, the others use the table.
        */
        Key mapKey(WPARAM virtualKey) noexcept
        {
            static const struct { WPARAM code; Key key; } KEYS[] = {
                {VK_BACK, Key::BACKSPACE}, {VK_TAB, Key::TAB}, {VK_RETURN, Key::ENTER}, {VK_ESCAPE, Key::ESCAPE},
                {VK_SPACE, Key::SPACE}, {VK_LEFT, Key::LEFT}, {VK_RIGHT, Key::RIGHT}, {VK_UP, Key::UP},
                {VK_DOWN, Key::DOWN}, {VK_HOME, Key::HOME}, {VK_END, Key::END}, {VK_DELETE, Key::DELETE_KEY},
                {VK_PRIOR, Key::PAGE_UP}, {VK_NEXT, Key::PAGE_DOWN}, {VK_INSERT, Key::INSERT}, {VK_APPS, Key::CONTEXT_MENU},
                {VK_MENU, Key::ALT}};

            if (virtualKey >= 'A' && virtualKey <= 'Z')
                return keyAfter(Key::A, static_cast<unsigned>(virtualKey - 'A'));
            if (virtualKey >= '0' && virtualKey <= '9')
                return keyAfter(Key::DIGIT_0, static_cast<unsigned>(virtualKey - '0'));
            if (virtualKey >= VK_F1 && virtualKey <= VK_F12)
                return keyAfter(Key::F1, static_cast<unsigned>(virtualKey - VK_F1));
            for (const auto& entry : KEYS)
            {
                if (entry.code == virtualKey)
                    return entry.key;
            }
            return Key::UNKNOWN;
        }

        // Символ: суррогатная пара UTF-16 склеивается, управляющие символы отбрасываются; modifiers — Alt у WM_SYSCHAR
        // A character: a UTF-16 surrogate pair is joined, control characters are dropped; modifiers — Alt for WM_SYSCHAR
        void translateCharacter(Win32Window& window, WPARAM wParam, std::uint8_t modifiers) noexcept
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
            event.modifiers = modifiers;
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

    /*
    ** Клавиши (WM_SYSKEYDOWN и WM_SYSKEYUP уходят и в DefWindowProc: Alt+F4 работает) и символы. Alt+буква
    ** (WM_SYSCHAR) — символ с Alt для букв доступа меню; в DefWindowProc он не идёт: там он искал бы меню
    ** окна HMENU, которого нет, и звучал бы сигнал ошибки. Alt+пробел — системное меню окна, как обычно.
    **
    ** Keys (WM_SYSKEYDOWN and WM_SYSKEYUP also go to DefWindowProc: Alt+F4 works) and characters. Alt+letter
    ** (WM_SYSCHAR) is a character with Alt for the menu access letters; it does not go to DefWindowProc: there it would look
    ** for the window HMENU menu, which does not exist, and the error beep would sound. Alt+space is the window system menu, as usual.
    */
    bool translateKeyboard(Win32Window& window, UINT message, WPARAM wParam)
    {
        Event event;
        std::uint8_t modifiers = static_cast<std::uint8_t>(currentModifiers() & ~MODIFIER_SHIFT);

        // WM_SYSCHAR приходит только при зажатом Alt / WM_SYSCHAR comes only with Alt held
        if (message == WM_SYSCHAR)
            modifiers = static_cast<std::uint8_t>(modifiers | MODIFIER_ALT);
        if (message == WM_CHAR || (message == WM_SYSCHAR && wParam != L' '))
        {
            translateCharacter(window, wParam, modifiers);
            return true;
        }
        if (message != WM_KEYDOWN && message != WM_KEYUP && message != WM_SYSKEYDOWN && message != WM_SYSKEYUP)
            return false;
        event.kind = EventKind::KEY_DOWN;
        if (message == WM_KEYUP || message == WM_SYSKEYUP)
            event.kind = EventKind::KEY_UP;
        event.key = mapKey(wParam);
        event.modifiers = currentModifiers();
        window.push(event);
        return message != WM_SYSKEYDOWN && message != WM_SYSKEYUP;
    }

    /*
    ** Меню окна с клавиатуры (SC_KEYMENU): одиночный Alt и F10 ведут в строку меню движка, Alt+буква — в её
    ** букву доступа. DefWindowProc тут вошёл бы в невидимый режим системного меню (следующие клавиши пропадают)
    ** или дал бы сигнал ошибки, поэтому сообщение поглощается. Alt+пробел (lParam = ' ') — системное меню, как обычно.
    **
    ** The window menu from the keyboard (SC_KEYMENU): a single Alt and F10 lead to the engine menu bar, Alt+letter — to its
    ** access letter. DefWindowProc here would enter the invisible system menu mode (the next keys get lost)
    ** or give the error beep, so the message is consumed. Alt+space (lParam = ' ') is the system menu, as usual.
    */
    bool swallowKeyMenu(UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        return message == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_KEYMENU && lParam != L' ';
    }
}
