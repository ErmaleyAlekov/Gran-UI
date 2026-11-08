#include "gran/platform/win32/win32_window.hpp"

#include <dwmapi.h>

namespace gran::platform::win32
{
    namespace
    {
        /*
        ** Новый размер; при перетаскивании края — синхронная перерисовка за noexcept-барьером и ожидание
        ** композиции DWM (Т-18). Без ожидания мышь давала по несколько WM_SIZE за одно обновление экрана: на каждое —
        ** пересоздание буферов и Present, до монитора доходил лишь каждый четвёртый кадр. На NVIDIA с аппаратными
        ** слоями (MPO) окно от такого потока на 1–5 с гасло — только на мониторе, запись экрана этого не видит.
        ** DwmFlush держит темп «один кадр — одна композиция», как у программ, которые рисуют в своём потоке.
        ** Ошибку DwmFlush (композиция выключена, сеанс без рабочего стола) не проверяем: тогда ждать нечего.
        **
        ** A new size; while dragging the edge — a synchronous repaint behind a noexcept barrier and waiting for the
        ** DWM composition (T-18). Without the wait the mouse gave several WM_SIZE per screen refresh: for each — buffer
        ** recreation and Present, only every fourth frame reached the monitor. On NVIDIA with hardware planes (MPO) the
        ** window went dark for 1–5 s from such a stream — on the monitor only, a screen recording does not see it.
        ** DwmFlush keeps the "one frame — one composition" pace, as programs drawing on their own thread have.
        ** A DwmFlush error (composition off, a session without a desktop) is not checked: then there is nothing to wait for.
        */
        void handleSize(Win32Window& self, LPARAM lParam) noexcept
        {
            Event event;

            event.kind = EventKind::RESIZE;
            event.width = LOWORD(lParam);
            event.height = HIWORD(lParam);
            event.scale = self.currentScale;
            self.push(event);
            if (!self.sizing || !self.liveResize)
                return;
            try
            {
                self.liveResize();
            }
            catch (...)
            {
                // Ошибку кадра покажет следующий обычный кадр цикла / The frame error shows up in the next regular loop frame
            }
            DwmFlush();
        }

        // Смена DPI: окно принимает размер, предложенный системой / A DPI change: the window takes the size suggested by the system
        void handleDpi(Win32Window& self, WPARAM wParam, LPARAM lParam) noexcept
        {
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            Event event;

            self.currentScale = static_cast<float>(HIWORD(wParam)) / 96.0f;
            SetWindowPos(self.window, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            event.kind = EventKind::DPI_CHANGED;
            event.scale = self.currentScale;
            self.push(event);
        }

        // Минимальный размер окна в пикселях с учётом DPI / The minimum window size in pixels considering DPI
        void handleMinSize(const Win32Window& self, LPARAM lParam) noexcept
        {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            RECT frame{0, 0, static_cast<LONG>(self.desc.minWidth * self.currentScale),
                static_cast<LONG>(self.desc.minHeight * self.currentScale)};

            if (self.desc.minWidth <= 0.0f && self.desc.minHeight <= 0.0f)
                return;
            if (self.desc.titleBar == TitleBar::SYSTEM)
                AdjustWindowRectExForDpi(&frame, static_cast<DWORD>(GetWindowLongPtrW(self.window, GWL_STYLE)), FALSE, 0,
                    GetDpiForWindow(self.window));
            if (self.desc.minWidth > 0.0f)
                info->ptMinTrackSize.x = frame.right - frame.left;
            if (self.desc.minHeight > 0.0f)
                info->ptMinTrackSize.y = frame.bottom - frame.top;
        }

        // Простые события без параметров / Simple events without parameters
        bool handleSimple(Win32Window& self, UINT message) noexcept
        {
            Event event;

            if (message == WM_CLOSE)
                event.kind = EventKind::CLOSE;
            else if (message == WM_SETFOCUS)
                event.kind = EventKind::FOCUS_GAINED;
            else if (message == WM_KILLFOCUS)
                event.kind = EventKind::FOCUS_LOST;
            else
                return false;
            self.push(event);
            return true;
        }
    }

    /*
    ** Сообщения ОС → события. Закрытие окна решает приложение (WM_CLOSE не уничтожает окно),
    ** фон стирается сплошным цветом окна (win32_paint.cpp), курсор ставится только над клиентской областью.
    **
    ** OS messages → events. The application decides on closing (WM_CLOSE does not destroy the window),
    ** the background is erased with the solid window color (win32_paint.cpp), the cursor is set only over the client area.
    */
    LRESULT Win32Window::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        LRESULT result = 0;

        if (handleFrameMessage(*this, message, wParam, lParam, result))
            return result;
        if (handleSimple(*this, message) || translateMouse(*this, message, wParam, lParam)
            || translateKeyboard(*this, message, wParam))
            return 0;
        if (message == WM_SIZE)
            handleSize(*this, lParam);
        else if (message == WM_DPICHANGED)
            handleDpi(*this, wParam, lParam);
        else if (message == WM_GETMINMAXINFO)
            handleMinSize(*this, lParam);
        else if (message == WM_ENTERSIZEMOVE || message == WM_EXITSIZEMOVE)
            sizing = message == WM_ENTERSIZEMOVE;
        else if (message == WM_ERASEBKGND)
        {
            paintBackground(*this, reinterpret_cast<HDC>(wParam));
            return 1;
        }
        else if (message == WM_SETCURSOR && handleSetCursor(*this, wParam, lParam))
            return TRUE;
        else if (message == WM_PAINT)
        {
            handlePaint(*this);
            return 0;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }
}
