#include "gran/platform/x11/x11_window.hpp"

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
}
