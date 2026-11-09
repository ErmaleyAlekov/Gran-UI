#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    // Исключения обработчика дерева не пересекают границу ОС; безопасный ответ — клиентская область.
    // Tree callback exceptions never cross the OS boundary; the safe answer is the client area.
    bool handleFrameMessage(Win32Window& self, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result) noexcept
    {
        // Системная рамка: только ручка размера внутри клиентской области / The system frame: only the size grip inside the client area
        if (self.desc.titleBar != TitleBar::CUSTOM && message == WM_NCHITTEST)
        {
            result = hitGrip(self, lParam, DefWindowProcW(self.window, message, wParam, lParam));
            return true;
        }
        if (self.desc.titleBar != TitleBar::CUSTOM)
            return false;
        result = 0;
        if (message == WM_NCCALCSIZE)
            fitClientRect(self, *reinterpret_cast<RECT*>(lParam));
        else if (message == WM_NCHITTEST)
        {
            result = HTCLIENT;
            try { result = hitFrame(self, lParam); }
            catch (...) { /* Клиентский ввод остаётся доступным. / Client input remains available. */ }
        }
        else if (message == WM_NCACTIVATE)
            result = DefWindowProcW(self.window, message, wParam, -1);
        else if (message == WM_DWMCOMPOSITIONCHANGED)
            configureFrame(self);
        else if (message != WM_NCPAINT)
            return translateFrameMouse(self, message, wParam, lParam, result);
        return true;
    }
}
