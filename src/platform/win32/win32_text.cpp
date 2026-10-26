#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::win32
{
    // UTF-8 → UTF-16 средствами ОС / UTF-8 → UTF-16 by the OS
    std::wstring toWide(const std::string& text)
    {
        int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
        std::wstring wide(static_cast<std::size_t>(length), L'\0');

        if (length > 0)
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(), length);
        return wide;
    }

    // UTF-16 (с нулём в конце) → UTF-8 / UTF-16 (null terminated) → UTF-8
    std::string toUtf8(const wchar_t* text)
    {
        int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
        std::string narrow;

        if (length <= 1)
            return narrow;
        narrow.resize(static_cast<std::size_t>(length));
        WideCharToMultiByte(CP_UTF8, 0, text, -1, narrow.data(), length, nullptr, nullptr);
        narrow.pop_back();
        return narrow;
    }

    // Per-monitor v2: окно само масштабируется под каждый монитор, ОС не растягивает его растром
    // Per-monitor v2: the window scales itself for each monitor, the OS does not stretch it as a bitmap
    void enableDpiAwareness() noexcept
    {
        static bool done = false;

        if (done)
            return;
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        done = true;
    }
}

namespace gran::platform
{
    // Фабрика окна Windows / The Windows window factory
    std::unique_ptr<Window> createWindow(const WindowDesc& desc)
    {
        return std::make_unique<win32::Win32Window>(desc);
    }
}
