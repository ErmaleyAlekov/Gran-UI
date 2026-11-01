#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    /*
    ** Курсор интерфейса запоминается всегда, а ставится только над клиентской областью. Кадр вызывает
    ** setCursor после каждой отрисовки; если указатель стоит на краю окна (своя рамка, HTLEFT и т. п.),
    ** немедленный SetCursor сменял бы курсор растягивания стрелкой — курсор мигал.
    **
    ** The interface cursor is always remembered, but set only over the client area. A frame calls
    ** setCursor after each render; if the pointer is on the window edge (custom frame, HTLEFT etc.),
    ** an immediate SetCursor would replace the resize cursor with an arrow — the cursor flickered.
    */
    void Win32Window::setCursor(CursorShape shape)
    {
        static const LPCWSTR CURSORS[] = {IDC_ARROW, IDC_HAND, IDC_IBEAM, IDC_SIZEALL, IDC_NO, IDC_HAND, IDC_SIZEWE, IDC_SIZENS};

        cursor = shape;
        if (cursorArea != HTCLIENT)
            return;
        SetCursor(LoadCursorW(nullptr, CURSORS[static_cast<std::size_t>(shape)]));
    }

    /*
    ** WM_SETCURSOR приходит при каждом движении указателя без захвата. Зона чужого (дочернего) окна
    ** считается «нигде»: курсор там ставит само дочернее окно. Вне клиента — false, и DefWindowProc
    ** ставит системный курсор зоны (стрелки растягивания у краёв).
    **
    ** WM_SETCURSOR arrives on every pointer move without capture. The area of another (child) window
    ** counts as "nowhere": the child window sets the cursor there itself. Outside the client — false, and
    ** DefWindowProc sets the system cursor of the area (resize arrows at the edges).
    */
    bool handleSetCursor(Win32Window& self, WPARAM wParam, LPARAM lParam) noexcept
    {
        self.cursorArea = HTNOWHERE;
        if (reinterpret_cast<HWND>(wParam) == self.window)
            self.cursorArea = LOWORD(lParam);
        if (self.cursorArea != HTCLIENT)
            return false;
        self.setCursor(self.cursor);
        return true;
    }
}
