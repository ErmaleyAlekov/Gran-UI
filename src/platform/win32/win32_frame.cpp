#include "gran/platform/win32/win32_window.hpp"

#include <dwmapi.h>
#include <cmath>
#include <utility>

namespace gran::platform::win32
{
    // Обработчик не вызывается до подключения маршрутизатора или во время замены дерева.
    // The callback is not called before router attachment or during tree replacement.
    void Win32Window::setFrameHitTest(std::function<WindowRole(Vec2)> callback)
    {
        frameHitTest = std::move(callback);
    }

    // Короткое расширение включает тень DWM; скругление — лишь пожелание Windows 11.
    // A small frame extension enables the DWM shadow; rounding is only a Windows 11 preference.
    void configureFrame(Win32Window& self) noexcept
    {
        BOOL dark = TRUE;

        if (self.desc.darkTitleBar)
            DwmSetWindowAttribute(self.window, 20, &dark, sizeof(dark));
        if (self.desc.titleBar != TitleBar::CUSTOM)
            return;
        MARGINS margins{1, 1, 1, 1};
        DWORD corner = 2;
        // На Windows 10 атрибут 33 не поддерживается: прямые углы допустимы.
        // Windows 10 does not support attribute 33: square corners are acceptable.
        DwmExtendFrameIntoClientArea(self.window, &margins);
        DwmSetWindowAttribute(self.window, 33, &corner, sizeof(corner));
    }

    // В развёрнутом окне невидимая рамка ОС выходит за рабочую область монитора.
    // In a maximized window the invisible OS frame extends beyond the monitor work area.
    void fitClientRect(const Win32Window& self, RECT& frame) noexcept
    {
        if (!IsZoomed(self.window))
            return;
        UINT dpi = GetDpiForWindow(self.window);
        int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        int horizontal = GetSystemMetricsForDpi(SM_CXFRAME, dpi) + padding;
        int vertical = GetSystemMetricsForDpi(SM_CYFRAME, dpi) + padding;

        frame.left += horizontal;
        frame.right -= horizontal;
        frame.top += vertical;
        frame.bottom -= vertical;
    }

    // Размер из XML — клиентский в обоих режимах; своей рамке поправка ОС не нужна.
    // The XML size is the client size in both modes; a custom frame needs no OS adjustment.
    void setInitialSize(Win32Window& self, DWORD style) noexcept
    {
        self.currentScale = static_cast<float>(GetDpiForWindow(self.window)) / 96.0f;
        RECT frame{0, 0, static_cast<LONG>(std::lround(self.desc.width * self.currentScale)),
            static_cast<LONG>(std::lround(self.desc.height * self.currentScale))};

        // Расширенный стиль (инструментальное окно, рамка диалога) меняет толщину заголовка и рамки
        // The extended style (a tool window, a dialog frame) changes the title bar and frame thickness
        DWORD extended = static_cast<DWORD>(GetWindowLongW(self.window, GWL_EXSTYLE));

        if (self.desc.titleBar == TitleBar::SYSTEM)
            AdjustWindowRectExForDpi(&frame, style, FALSE, extended, GetDpiForWindow(self.window));
        SetWindowPos(self.window, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        configureFrame(self);
    }
}
