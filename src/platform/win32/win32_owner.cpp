#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    /*
    ** Обычное окно — WS_OVERLAPPEDWINDOW; диалог и инструментальное — заголовок с крестиком без
    ** свёртки и развёртки (их сворачивает владелец). Без изменения размера рамка не тянется.
    **
    ** A normal window is WS_OVERLAPPEDWINDOW; a dialog and a tool one have a title bar with the cross without
    ** minimize and maximize (the owner minimizes them). Without resizing the frame does not stretch.
    */
    DWORD windowStyle(const WindowDesc& desc) noexcept
    {
        DWORD style = WS_OVERLAPPEDWINDOW;

        if (desc.kind != WindowKind::NORMAL)
            style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
        if (!desc.resizable)
            style &= ~static_cast<DWORD>(WS_THICKFRAME | WS_MAXIMIZEBOX);
        return style;
    }

    // Диалог — рамка без значка в заголовке; инструментальное — узкий заголовок и нет кнопки в панели задач
    // A dialog — a frame without an icon in the title bar; a tool one — a narrow title bar and no taskbar button
    DWORD windowExStyle(const WindowDesc& desc) noexcept
    {
        if (desc.kind == WindowKind::DIALOG)
            return WS_EX_DLGMODALFRAME;
        if (desc.kind == WindowKind::TOOL)
            return WS_EX_TOOLWINDOW;
        return 0;
    }

    /*
    ** Выключенное окно не получает мышь и клавиатуру; щелчок по нему Windows переводит
    ** на его активное подчинённое окно (модальное) и подсвечивает его. Возврат EnableWindow —
    ** прежнее состояние, а не успех, поэтому не проверяется.
    **
    ** A disabled window gets neither the mouse nor the keyboard; Windows moves a click on it
    ** to its active owned window (the modal one) and flashes it. The EnableWindow return value is
    ** the former state, not a success, so it is not checked.
    */
    void Win32Window::setEnabled(bool enabled)
    {
        EnableWindow(window, static_cast<BOOL>(enabled));
    }

    /*
    ** Свёрнутое окно разворачивается, затем получает фокус. SetForegroundWindow может отказать,
    ** если программа сейчас не на переднем плане (правило Windows против кражи фокуса), — тогда
    ** окно мигает в панели задач; это ожидаемо, поэтому результат не проверяется.
    **
    ** A minimized window is restored, then gets the focus. SetForegroundWindow may refuse
    ** if the program is not in the foreground now (the Windows rule against focus stealing) — then
    ** the window flashes in the taskbar; this is expected, so the result is not checked.
    */
    void Win32Window::activate()
    {
        if (IsIconic(window))
            ShowWindow(window, SW_RESTORE);
        SetForegroundWindow(window);
    }
}
