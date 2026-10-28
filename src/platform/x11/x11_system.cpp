#include "gran/platform/x11/x11_window.hpp"
#include <X11/cursorfont.h>

namespace gran::platform::x11
{
    // Vulkan получает XID и Display отдельно, без скрытых приведений структур.
    // Vulkan receives XID and Display separately, without hidden structure casts.
    void* X11Window::nativeHandle() const { return reinterpret_cast<void*>(static_cast<std::uintptr_t>(_native.window)); }
    void* X11Window::nativeDisplay() const { return _native.display; }
    float X11Window::scale() const { return _scale; }

    // Окно отображается после завершения первого кадра.
    // Map the window after completing the first frame.
    void X11Window::show()
    {
        XMapWindow(_native.display, _native.window);
        XFlush(_native.display);
    }

    // Стандартные курсоры предоставляет сервер, дополнительной библиотеки курсоров нет.
    // The server supplies standard cursors, without an additional cursor library.
    void X11Window::setCursor(CursorShape shape)
    {
        static const unsigned shapes[] = {XC_left_ptr, XC_hand2, XC_xterm, XC_fleur, XC_X_cursor, XC_hand1, XC_sb_h_double_arrow, XC_sb_v_double_arrow};
        unsigned index = static_cast<unsigned>(shape);
        if (index >= sizeof(shapes) / sizeof(shapes[0])) index = 0;
        ::Cursor cursor = XCreateFontCursor(_native.display, shapes[index]);
        XDefineCursor(_native.display, _native.window, cursor);
        if (_native.cursor != 0) XFreeCursor(_native.display, _native.cursor);
        _native.cursor = cursor;
    }
}
