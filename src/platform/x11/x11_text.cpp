#include "gran/platform/x11/x11_window.hpp"
#include "gran/utils/utf8.hpp"
#include <vector>

namespace gran::platform::x11
{
    // XIM возвращает UTF-8; длинные составные последовательности повторяются с нужной ёмкостью.
    // XIM returns UTF-8; long composed sequences are retried with the required capacity.
    void X11Window::text(XKeyEvent& native)
    {
        std::vector<char> bytes(64);
        KeySym symbol = 0;
        Status status = 0;
        int count = 0;
        if (_native.input != nullptr)
        {
            count = Xutf8LookupString(_native.input, &native, bytes.data(), static_cast<int>(bytes.size()), &symbol, &status);
            if (status == XBufferOverflow && count > 0 && count <= 1048576)
            {
                bytes.resize(static_cast<std::size_t>(count) + 1);
                count = Xutf8LookupString(_native.input, &native, bytes.data(), static_cast<int>(bytes.size()), &symbol, &status);
            }
            if (status != XLookupChars && status != XLookupBoth) return;
        }
        else count = XLookupString(&native, bytes.data(), static_cast<int>(bytes.size()), &symbol, nullptr);
        if (count <= 0 || static_cast<std::size_t>(count) > bytes.size()) return;
        std::string_view value(bytes.data(), static_cast<std::size_t>(count));
        std::size_t offset = 0;
        while (offset < value.size())
        {
            Event event; event.kind = EventKind::CHARACTER; event.character = decodeUtf8(value, offset);
            event.modifiers = static_cast<std::uint8_t>(modifiers(native.state) & ~MODIFIER_SHIFT);
            if (event.character >= 32 && event.character != 127) _events.push_back(event);
        }
    }
}
