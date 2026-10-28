#include "gran/platform/x11/x11_window.hpp"
#include <X11/keysym.h>

namespace gran::platform::x11
{
    // Служебные клавиши и латинские shortcuts независимо от регистра.
    // Navigation keys and Latin shortcuts independent of case.
    Key X11Window::key(KeySym symbol)
    {
        switch (symbol)
        {
            case XK_BackSpace: return Key::BACKSPACE; case XK_Tab: case XK_ISO_Left_Tab: return Key::TAB;
            case XK_Return: case XK_KP_Enter: return Key::ENTER; case XK_Escape: return Key::ESCAPE;
            case XK_space: return Key::SPACE; case XK_Left: return Key::LEFT; case XK_Right: return Key::RIGHT;
            case XK_Up: return Key::UP; case XK_Down: return Key::DOWN; case XK_Home: return Key::HOME;
            case XK_End: return Key::END; case XK_Delete: return Key::DELETE_KEY;
            case XK_Page_Up: return Key::PAGE_UP; case XK_Page_Down: return Key::PAGE_DOWN;
            case XK_a: case XK_A: return Key::A; case XK_c: case XK_C: return Key::C;
            case XK_v: case XK_V: return Key::V; case XK_x: case XK_X: return Key::X;
            case XK_z: case XK_Z: return Key::Z; default: return Key::UNKNOWN;
        }
    }

    /*
    ** Раскладка без латинской группы (только «ru»): у клавиши A нет символа XK_a, и Ctrl+C не узнать
    ** по символу. Как Qt и GTK, берём физическую клавишу: коды <AC01>, <AB01>…<AB04> одинаковы
    ** в наборах evdev и xfree86 (XWayland, Xorg).
    **
    ** A layout without a Latin group ("ru" only): the A key has no XK_a symbol, and Ctrl+C cannot be recognized
    ** by the symbol. Like Qt and GTK, take the physical key: the <AC01>, <AB01>…<AB04> codes are the same
    ** in the evdev and xfree86 sets (XWayland, Xorg).
    */
    Key X11Window::physicalKey(unsigned keycode)
    {
        switch (keycode)
        {
            case 38: return Key::A;
            case 52: return Key::Z;
            case 53: return Key::X;
            case 54: return Key::C;
            case 55: return Key::V;
            default: return Key::UNKNOWN;
        }
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

    // Сначала клавиша, затем символы: обработчик shortcuts может подавить обычное редактирование.
    // First the key, then characters: shortcut handlers can suppress ordinary editing.
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
        if (down && !(native.state & (ControlMask | Mod1Mask))) text(native);
    }
}
