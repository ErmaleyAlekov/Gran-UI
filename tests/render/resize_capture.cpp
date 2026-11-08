#include "resize_capture.hpp"

#include <cstddef>

namespace render_tests
{
    namespace
    {
        // Белый — все каналы не ниже 250, чёрный — не выше 4: так DWM заливает непрорисованное; тёмный фон
        // витрины (#0f1117) и серая подложка под эти пороги не попадают
        // White — all channels at least 250, black — at most 4: this is how DWM fills the unpainted; the dark
        // showcase background (#0f1117) and the gray backdrop do not fall under these thresholds
        constexpr std::uint8_t WHITE_FLOOR = 250;
        constexpr std::uint8_t BLACK_CEIL = 4;
        const wchar_t BACKDROP_CLASS[] = L"gran-test-backdrop";
    }

    // Окно перечисления: самое большое видимое окно верхнего уровня / The enumeration callback: the largest visible top-level window
    BOOL CALLBACK pickLargest(HWND window, LPARAM found) noexcept
    {
        HWND* best = reinterpret_cast<HWND*>(found);
        RECT area{};
        RECT bestArea{};

        if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr || !GetWindowRect(window, &area))
            return TRUE;
        if (*best != nullptr)
            GetWindowRect(*best, &bestArea);
        if ((area.right - area.left) * (area.bottom - area.top)
            > (bestArea.right - bestArea.left) * (bestArea.bottom - bestArea.top))
            *best = window;
        return TRUE;
    }

    // Главное окно приложения в этом потоке (до показа — nullptr) / The application main window in this thread (before showing — nullptr)
    HWND threadWindow()
    {
        HWND best = nullptr;

        EnumThreadWindows(GetCurrentThreadId(), pickLargest, reinterpret_cast<LPARAM>(&best));
        return best;
    }

    // Класс подложки регистрируется при первом вызове; окно без рамки, не забирает фокус
    // The backdrop class is registered at the first call; a borderless window that does not take focus
    HWND createBackdrop(HWND above, const RECT& area)
    {
        static bool registered = false;
        WNDCLASSW windowClass{};

        if (!registered)
        {
            windowClass.lpfnWndProc = DefWindowProcW;
            windowClass.hInstance = GetModuleHandleW(nullptr);
            windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH));
            windowClass.lpszClassName = BACKDROP_CLASS;
            RegisterClassW(&windowClass);
            registered = true;
        }
        HWND backdrop = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, BACKDROP_CLASS, L"", WS_POPUP,
            area.left, area.top, area.right - area.left, area.bottom - area.top, nullptr, nullptr,
            GetModuleHandleW(nullptr), nullptr);
        // hWndInsertAfter = окно теста: подложка встаёт сразу под ним
        // hWndInsertAfter = the test window: the backdrop goes right below it
        SetWindowPos(backdrop, above, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        UpdateWindow(backdrop);
        return backdrop;
    }

    // Столбец «плохой», если больше половины его пикселей чисто белые или чисто чёрные
    // A column is "bad" if more than half of its pixels are pure white or pure black
    int badColumns(const std::uint8_t* pixels, unsigned pitch, int width, int height)
    {
        int columns = 0;

        for (int x = 0; x < width; ++x)
        {
            int bad = 0;
            for (int y = 0; y < height; ++y)
            {
                const std::uint8_t* pixel = pixels + static_cast<std::size_t>(y) * pitch + static_cast<std::size_t>(x) * 4;
                bool white = pixel[0] >= WHITE_FLOOR && pixel[1] >= WHITE_FLOOR && pixel[2] >= WHITE_FLOOR;
                bool black = pixel[0] <= BLACK_CEIL && pixel[1] <= BLACK_CEIL && pixel[2] <= BLACK_CEIL;
                if (white || black)
                    ++bad;
            }
            if (bad * 2 > height)
                ++columns;
        }
        return columns;
    }
}
