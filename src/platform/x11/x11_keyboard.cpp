#include "gran/platform/x11/x11_window.hpp"
#include <X11/keysym.h>

namespace gran::platform::x11
{
    /*
    ** Служебные клавиши, латинские буквы без учёта регистра, цифры и F1–F12: у букв, цифр и F-клавиш
    ** коды KeySym идут подряд, как и в Key, — сдвиг от первой.
    **
    ** The service keys, the Latin letters regardless of case, the digits and F1–F12: the KeySym codes of the letters,
    ** the digits and the F keys go in a row, as in Key, — an offset from the first one.
    */
    Key X11Window::key(KeySym symbol)
    {
        if (symbol >= XK_a && symbol <= XK_z) return keyAfter(Key::A, static_cast<unsigned>(symbol - XK_a));
        if (symbol >= XK_A && symbol <= XK_Z) return keyAfter(Key::A, static_cast<unsigned>(symbol - XK_A));
        if (symbol >= XK_0 && symbol <= XK_9) return keyAfter(Key::DIGIT_0, static_cast<unsigned>(symbol - XK_0));
        if (symbol >= XK_F1 && symbol <= XK_F12) return keyAfter(Key::F1, static_cast<unsigned>(symbol - XK_F1));
        switch (symbol)
        {
            case XK_BackSpace: return Key::BACKSPACE; case XK_Tab: case XK_ISO_Left_Tab: return Key::TAB;
            case XK_Return: case XK_KP_Enter: return Key::ENTER; case XK_Escape: return Key::ESCAPE;
            case XK_space: return Key::SPACE; case XK_Left: return Key::LEFT; case XK_Right: return Key::RIGHT;
            case XK_Up: return Key::UP; case XK_Down: return Key::DOWN; case XK_Home: return Key::HOME;
            case XK_End: return Key::END; case XK_Delete: return Key::DELETE_KEY;
            case XK_Page_Up: return Key::PAGE_UP; case XK_Page_Down: return Key::PAGE_DOWN;
            case XK_Insert: return Key::INSERT; case XK_Menu: return Key::CONTEXT_MENU;
            case XK_Alt_L: case XK_Alt_R: return Key::ALT;
            default: return Key::UNKNOWN;
        }
    }

    /*
    ** Раскладка без латинской группы (только «ru»): у клавиши A нет символа XK_a, и Ctrl+C не узнать
    ** по символу. Как Qt и GTK, берём физическую клавишу: коды рядов <AD01>…<AD10> (Q…P), <AC01>…<AC09>
    ** (A…L), <AB01>…<AB07> (Z…M) и цифр <AE01>…<AE10> одинаковы в наборах evdev и xfree86 (XWayland, Xorg).
    **
    ** A layout without a Latin group ("ru" only): the A key has no XK_a symbol, and Ctrl+C cannot be recognized
    ** by the symbol. Like Qt and GTK, take the physical key: the codes of the rows <AD01>…<AD10> (Q…P), <AC01>…<AC09>
    ** (A…L), <AB01>…<AB07> (Z…M) and the digits <AE01>…<AE10> are the same in the evdev and xfree86 sets (XWayland, Xorg).
    */
    Key X11Window::physicalKey(unsigned keycode)
    {
        static const char ROWS[] = "QWERTYUIOP....ASDFGHJKL.....ZXCVBNM";

        if (keycode >= 10 && keycode <= 18)
            return keyAfter(Key::DIGIT_1, keycode - 10);
        if (keycode == 19)
            return Key::DIGIT_0;
        if (keycode < 24 || keycode - 24 >= sizeof(ROWS) - 1 || ROWS[keycode - 24] == '.')
            return Key::UNKNOWN;
        return keyAfter(Key::A, static_cast<unsigned>(ROWS[keycode - 24] - 'A'));
    }

    // X11 и движок имеют разные значения битов Control и Alt.
    // X11 and the engine use different bit values for Control and Alt.
    std::uint8_t X11Window::modifiers(unsigned state)
    {
        std::uint8_t result = 0;
        if (state & ShiftMask) result |= MODIFIER_SHIFT;
        if (state & ControlMask) result |= MODIFIER_CONTROL;
        if (state & Mod1Mask) result |= MODIFIER_ALT;
        return result;
    }

    // Сначала клавиша, затем символы: обработчик shortcuts может подавить обычное редактирование. Символы с Alt
    // (без Ctrl) тоже идут — как буквы доступа меню; в поле ввода их не вставляет маршрутизатор.
    // First the key, then characters: shortcut handlers can suppress ordinary editing. The characters with Alt
    // (without Ctrl) go too — as the menu access letters; the router does not insert them into an input.
    void X11Window::keyboard(XKeyEvent& native, bool down)
    {
        Event event;
        event.kind = EventKind::KEY_UP;
        if (down) event.kind = EventKind::KEY_DOWN;
        event.key = key(XLookupKeysym(&native, 0));
        if (event.key == Key::UNKNOWN)
            event.key = physicalKey(native.keycode);
        event.modifiers = modifiers(native.state);
        _events.push_back(event);
        if (down && !(native.state & ControlMask)) text(native);
    }
}
