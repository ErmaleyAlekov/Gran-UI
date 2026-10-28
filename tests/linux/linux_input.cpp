#include "linux_tests.hpp"
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

namespace linux_tests
{
    namespace
    {
        // XSendEvent проверяет весь путь X-сервер → оконный цикл → Event, включая XIM.
        // Буквы — физическими кодами (<AC01> = 38, <AB03> = 54): тест не зависит от раскладки.
        // XSendEvent tests the whole X-server → window loop → Event path, including XIM.
        // Letters go by physical codes (<AC01> = 38, <AB03> = 54): the test does not depend on the layout.
        void sendKey(gran::platform::Window& window, unsigned keycode, unsigned state)
        {
            Display* display = static_cast<Display*>(window.nativeDisplay());
            XEvent event{};
            event.xkey.type = KeyPress; event.xkey.display = display;
            event.xkey.window = reinterpret_cast<std::uintptr_t>(window.nativeHandle());
            event.xkey.root = DefaultRootWindow(display); event.xkey.same_screen = True;
            event.xkey.keycode = keycode; event.xkey.state = state;
            require(XSendEvent(display, event.xkey.window, False, KeyPressMask, &event) != 0, "XSendEvent key");
        }
    }

    // Набор сочетает печатный символ, служебную клавишу и shortcut без CHARACTER.
    // The sequence combines a printable character, navigation key and shortcut without CHARACTER.
    void input(gran::platform::Window& window)
    {
        Display* display = static_cast<Display*>(window.nativeDisplay());
        // В латинской раскладке Shift+<AC01> — ровно «A»; в другой — любой печатный символ
        // In a Latin layout Shift+<AC01> is exactly "A"; in another one — any printable character
        bool latin = XkbKeycodeToKeysym(display, 38, 0, 0) == XK_a;
        sendKey(window, 38, ShiftMask);
        sendKey(window, XKeysymToKeycode(display, XK_Return), 0);
        sendKey(window, 54, ControlMask);
        XSync(display, False);
        bool character = false; bool enter = false; bool control = false;
        gran::platform::Event event;
        while (window.pollEvent(event))
        {
            if (event.kind == gran::platform::EventKind::CHARACTER)
            { require(event.character == U'A' || (!latin && event.character >= 0x20), "XIM uppercase character"); character = true; }
            if (event.kind != gran::platform::EventKind::KEY_DOWN) continue;
            enter |= event.key == gran::platform::Key::ENTER;
            control |= event.key == gran::platform::Key::C && (event.modifiers & gran::platform::MODIFIER_CONTROL);
        }
        require(character && enter && control, "X11 keyboard translation");
    }
}
