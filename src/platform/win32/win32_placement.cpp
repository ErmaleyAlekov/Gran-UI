#include "gran/platform/win32/win32_window.hpp"

#include <algorithm>

namespace gran::platform::win32
{
    namespace
    {
        /*
        ** Один шаг: окно по центру прямоугольника владельца (или рабочей области своего монитора)
        ** и целиком в рабочей области монитора, на который попал центр (панель задач не закрывается).
        **
        ** One step: the window at the center of the owner rectangle (or of its own monitor work area)
        ** and entirely inside the work area of the monitor the center fell on (the taskbar is not covered).
        */
        void centerOnce(Win32Window& self, HWND owner) noexcept
        {
            MONITORINFO monitor{};
            RECT target{};
            RECT frame{};

            monitor.cbSize = sizeof(monitor);
            GetMonitorInfoW(MonitorFromWindow(self.window, MONITOR_DEFAULTTONEAREST), &monitor);
            target = monitor.rcWork;
            if (owner != nullptr && GetWindowRect(owner, &target))
                GetMonitorInfoW(MonitorFromRect(&target, MONITOR_DEFAULTTONEAREST), &monitor);
            GetWindowRect(self.window, &frame);
            LONG width = frame.right - frame.left;
            LONG height = frame.bottom - frame.top;
            LONG x = target.left + (target.right - target.left - width) / 2;
            LONG y = target.top + (target.bottom - target.top - height) / 2;
            x = std::max(monitor.rcWork.left, std::min(x, monitor.rcWork.right - width));
            y = std::max(monitor.rcWork.top, std::min(y, monitor.rcWork.bottom - height));
            SetWindowPos(self.window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    /*
    ** DEFAULT оставляет место ОС. Иначе — два шага: переход на монитор владельца может прислать
    ** WM_DPICHANGED и изменить размер окна, второй шаг центрирует уже с новым размером.
    **
    ** DEFAULT leaves the place to the OS. Otherwise — two steps: moving to the owner monitor may send
    ** WM_DPICHANGED and change the window size, the second step centers with the new size.
    */
    void placeWindow(Win32Window& self, HWND owner) noexcept
    {
        if (self.desc.position == WindowPosition::DEFAULT)
            return;
        if (self.desc.position == WindowPosition::CENTER)
            owner = nullptr;
        centerOnce(self, owner);
        centerOnce(self, owner);
    }
}
