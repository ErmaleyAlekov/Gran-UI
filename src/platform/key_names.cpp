#include "gran/platform/window.hpp"

namespace gran::platform
{
    /*
    ** Имена в порядке Key. Служебные — как свойство key у KeyboardEvent в браузерах ("ArrowLeft", "Enter"),
    ** буквы — заглавные латинские, цифры — "0"…"9": приложение сравнивает их без учёта раскладки.
    **
    ** The names in the Key order. The service keys are as the key property of a browser KeyboardEvent ("ArrowLeft", "Enter"),
    ** the letters are capital Latin ones, the digits are "0"…"9": the application compares them regardless of the layout.
    */
    const char* keyName(Key key) noexcept
    {
        static const char* const NAMES[] = {"Unidentified", "Backspace", "Tab", "Enter", "Escape", "Space",
            "ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown", "Home", "End", "Delete", "PageUp", "PageDown",
            "Insert", "ContextMenu",
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
            "U", "V", "W", "X", "Y", "Z",
            "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
            "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "Alt"};
        std::size_t index = static_cast<std::size_t>(key);

        if (index >= sizeof(NAMES) / sizeof(NAMES[0]))
            return NAMES[0];
        return NAMES[index];
    }

    // Сдвиг внутри ряда; выход за F12 — UNKNOWN / An offset inside a row; going past F12 — UNKNOWN
    Key keyAfter(Key first, unsigned offset) noexcept
    {
        unsigned code = static_cast<unsigned>(first) + offset;

        if (code > static_cast<unsigned>(Key::F12))
            return Key::UNKNOWN;
        return static_cast<Key>(code);
    }
}
