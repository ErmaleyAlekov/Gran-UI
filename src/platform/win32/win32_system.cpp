#include "gran/platform/win32/win32_window.hpp"

#include <cstring>

namespace gran::platform::win32
{
    // Масштаб DPI монитора окна / The DPI scale of the window monitor
    float Win32Window::scale() const
    {
        return currentScale;
    }

    // Показ и активация окна / Showing and activating the window
    void Win32Window::show()
    {
        int command = SW_SHOW;

        if (startMaximized)
            command = SW_SHOWMAXIMIZED;
        ShowWindow(window, command);
        UpdateWindow(window);
    }

    // Текст буфера обмена (CF_UNICODETEXT) или пустая строка / The clipboard text (CF_UNICODETEXT) or an empty string
    std::string Win32Window::clipboardText()
    {
        std::string text;

        if (!OpenClipboard(window))
            return text;
        HANDLE data = GetClipboardData(CF_UNICODETEXT);
        const wchar_t* locked = nullptr;
        if (data != nullptr)
            locked = static_cast<const wchar_t*>(GlobalLock(data));
        if (locked != nullptr)
        {
            text = toUtf8(locked);
            GlobalUnlock(data);
        }
        CloseClipboard();
        return text;
    }

    // Память для буфера обмена переходит во владение ОС только при успехе SetClipboardData
    // The clipboard memory passes into the OS ownership only when SetClipboardData succeeds
    void Win32Window::setClipboardText(const std::string& text)
    {
        std::wstring wide = toWide(text);
        std::size_t bytes = (wide.size() + 1) * sizeof(wchar_t);

        if (!OpenClipboard(window))
            return;
        EmptyClipboard();
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
        void* locked = nullptr;
        if (memory != nullptr)
            locked = GlobalLock(memory);
        if (locked != nullptr)
        {
            std::memcpy(locked, wide.c_str(), bytes);
            GlobalUnlock(memory);
            if (SetClipboardData(CF_UNICODETEXT, memory) != nullptr)
                memory = nullptr;
        }
        if (memory != nullptr)
            GlobalFree(memory);
        CloseClipboard();
    }
}
