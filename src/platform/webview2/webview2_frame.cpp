#include "gran/platform/webview2/internal/webview2_host.hpp"

#include <cmath>

namespace gran::platform::webview2
{
    namespace
    {
        const wchar_t FRAME_CLASS[] = L"GranWebViewFrame";

        // Рамка ничего не рисует сама: обработка сообщений — по умолчанию / The frame draws nothing itself: the message handling is the default one
        bool registerOnce(HINSTANCE instance) noexcept
        {
            WNDCLASSEXW frameClass{};

            frameClass.cbSize = sizeof(frameClass);
            frameClass.lpfnWndProc = DefWindowProcW;
            frameClass.hInstance = instance;
            frameClass.lpszClassName = FRAME_CLASS;
            return RegisterClassExW(&frameClass) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
        }

        // Класс окна-рамки регистрируется один раз на процесс / The frame window class is registered once per process
        bool registerFrameClass(HINSTANCE instance) noexcept
        {
            static const bool READY = registerOnce(instance);

            return READY;
        }
    }

    /*
    ** WS_CLIPSIBLINGS — рамки нескольких WebView не рисуют друг по другу; WS_CLIPCHILDREN — рамка
    ** не закрашивает окно браузера. Создаётся скрытой: покажет её первый setVisible(true).
    **
    ** WS_CLIPSIBLINGS — the frames of several WebViews do not draw over each other; WS_CLIPCHILDREN — the frame
    ** does not paint over the browser window. It is created hidden: the first setVisible(true) shows it.
    */
    HWND createFrameWindow(HWND parent) noexcept
    {
        HINSTANCE instance = GetModuleHandleW(nullptr);

        if (!registerFrameClass(instance))
            return nullptr;
        return CreateWindowExW(0, FRAME_CLASS, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
            0, 0, 0, 0, parent, nullptr, instance, nullptr);
    }

    // Края округляются отдельно: соседние блоки встают встык без щели в пиксель / The edges are rounded separately: neighbouring boxes meet without a one-pixel gap
    RECT toRect(const Rect& rect) noexcept
    {
        RECT result;

        result.left = static_cast<LONG>(std::lround(rect.x));
        result.top = static_cast<LONG>(std::lround(rect.y));
        result.right = static_cast<LONG>(std::lround(rect.x + rect.width));
        result.bottom = static_cast<LONG>(std::lround(rect.y + rect.height));
        return result;
    }
}
