#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    // ShowWindow сохраняет системное размещение, включая Aero Snap и восстановление.
    // ShowWindow preserves system placement, including Aero Snap and restoration.
    void Win32Window::minimize()
    {
        ShowWindow(window, SW_MINIMIZE);
    }

    // Фиксированное окно не разворачивается даже по команде разметки.
    // A fixed-size window cannot be maximized even by a markup command.
    void Win32Window::maximize()
    {
        if (desc.resizable)
            ShowWindow(window, SW_MAXIMIZE);
    }

    // Система возвращает прежний размер и положение.
    // The system restores the previous size and position.
    void Win32Window::restore()
    {
        ShowWindow(window, SW_RESTORE);
    }

    // Закрытие проходит через ту же очередь событий, что Alt+F4 и системное меню.
    // Closing uses the same event queue as Alt+F4 and the system menu.
    void Win32Window::close()
    {
        PostMessageW(window, WM_CLOSE, 0, 0);
    }

    // Источник истины — размещение HWND, в том числе после двойного щелчка заголовка.
    // The HWND placement is authoritative, including after a title bar double click.
    bool Win32Window::isMaximized() const
    {
        return IsZoomed(window) != FALSE;
    }
}
