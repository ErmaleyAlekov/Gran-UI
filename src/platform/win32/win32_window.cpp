#include "gran/platform/win32/win32_window.hpp"
#include "gran/platform/win32/internal/drop_target.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <dwmapi.h>

#include <cmath>

namespace gran::platform::win32
{
    namespace
    {
        // Имя класса окна / The window class name
        const wchar_t* const CLASS_NAME = L"GranUiWindow";

        // Оконная процедура: указатель на объект хранится в GWLP_USERDATA с WM_NCCREATE
        // The window procedure: the object pointer is stored in GWLP_USERDATA since WM_NCCREATE
        LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            if (message == WM_NCCREATE)
            {
                auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
                static_cast<Win32Window*>(create->lpCreateParams)->window = window;
            }
            auto* self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
            if (self == nullptr)
                return DefWindowProcW(window, message, wParam, lParam);
            if (message == WM_GETOBJECT)
                return answerAccessRequest(*self, wParam, lParam);
            return self->handleMessage(message, wParam, lParam);
        }

        // Класс регистрируется один раз; фон не задан — нет белой вспышки до первого кадра
        // The class is registered once; no background is set — no white flash before the first frame
        void registerClass()
        {
            static bool registered = false;
            WNDCLASSEXW windowClass{};

            if (registered)
                return;
            windowClass.cbSize = sizeof(windowClass);
            // CS_DBLCLKS: второе нажатие в пределах времени двойного щелчка приходит как WM_LBUTTONDBLCLK
            // CS_DBLCLKS: the second press within the double click time comes as WM_LBUTTONDBLCLK
            windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            windowClass.lpfnWndProc = windowProcedure;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            windowClass.lpszClassName = CLASS_NAME;
            RegisterClassExW(&windowClass);
            registered = true;
        }
    }

    /*
    ** Окно создаётся скрытым, затем получает размер с учётом DPI своего монитора
    ** (AdjustWindowRectExForDpi) и, если просили, тёмный заголовок. Показ — после первого кадра.
    **
    ** The window is created hidden, then gets its size with the DPI of its monitor
    ** (AdjustWindowRectExForDpi) and, if asked, a dark title bar. Showing — after the first frame.
    */
    Win32Window::Win32Window(const WindowDesc& windowDesc)
        : desc(windowDesc)
    {
        DWORD style = WS_OVERLAPPEDWINDOW;
        BOOL dark = TRUE;

        enableDpiAwareness();
        registerClass();
        if (!desc.resizable)
            style &= ~static_cast<DWORD>(WS_THICKFRAME | WS_MAXIMIZEBOX);
        CreateWindowExW(0, CLASS_NAME, toWide(desc.title).c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
            nullptr, nullptr, GetModuleHandleW(nullptr), this);
        if (window == nullptr)
            throw GranException(ErrorCode::PLATFORM, i18n::format(i18n::Message::PLATFORM_WINDOW_FAILED,
                {std::to_string(GetLastError())}));
        currentScale = static_cast<float>(GetDpiForWindow(window)) / 96.0f;
        RECT frame{0, 0, static_cast<LONG>(std::lround(desc.width * currentScale)),
            static_cast<LONG>(std::lround(desc.height * currentScale))};
        AdjustWindowRectExForDpi(&frame, style, FALSE, 0, GetDpiForWindow(window));
        SetWindowPos(window, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        if (desc.darkTitleBar)
            DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
    }

    // Окно уничтожается; указатель на объект из него убирается до этого / The window is destroyed; the object pointer is removed from it before that
    Win32Window::~Win32Window()
    {
        if (window == nullptr)
            return;
        disconnectAccess(*this);
        // Приёмник перетаскивания снимается до уничтожения окна; ссылку ОС отпустит сама
        // The drag-and-drop receiver is revoked before the window is destroyed; the OS releases its reference itself
        if (dropTarget)
        {
            RevokeDragDrop(window);
            dropTarget->detach();
        }
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        DestroyWindow(window);
    }

    // HWND для цепочки обмена / The HWND for the swap chain
    void* Win32Window::nativeHandle() const
    {
        return window;
    }
}
