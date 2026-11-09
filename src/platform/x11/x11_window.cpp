#include "gran/platform/x11/x11_window.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include <algorithm>
#include <clocale>
#include <fcntl.h>
#include <unistd.h>

namespace gran::platform
{
    // Фабрика не требует графического SDK для создания самого окна.
    // The factory needs no graphics SDK to create the window itself.
    std::unique_ptr<Window> createWindow(const WindowDesc& desc)
    {
        return std::make_unique<x11::X11Window>(desc);
    }
}

namespace gran::platform::x11
{
    // XIC и XIM уничтожаются раньше окна и соединения с сервером.
    // Destroy XIC and XIM before the window and server connection.
    Native::~Native()
    {
        if (input != nullptr) XDestroyIC(input);
        if (method != nullptr) XCloseIM(method);
        if (display != nullptr && cursor != 0) XFreeCursor(display, cursor);
        if (display != nullptr && window != 0) XDestroyWindow(display, window);
        if (display != nullptr) XCloseDisplay(display);
        if (wakeRead >= 0) ::close(wakeRead);
        if (wakeWrite >= 0) ::close(wakeWrite);
    }

    // Окно остаётся скрытым до первого кадра, как в Windows.
    // The window stays hidden until the first frame, as on Windows.
    X11Window::X11Window(const WindowDesc& desc)
    {
        _native.display = XOpenDisplay(nullptr);
        if (_native.display == nullptr)
            throw GranException(ErrorCode::PLATFORM, i18n::format(i18n::Message::PLATFORM_WINDOW_FAILED, {"XOpenDisplay / DISPLAY"}));
        int wakePipe[2];
        if (pipe2(wakePipe, O_CLOEXEC | O_NONBLOCK) != 0)
            throw GranException(ErrorCode::PLATFORM, i18n::format(i18n::Message::PLATFORM_WINDOW_FAILED, {"pipe2"}));
        _native.wakeRead = wakePipe[0]; _native.wakeWrite = wakePipe[1];
        create(desc);
        configure(desc);
        applyOwner(desc);
        place(desc);
        inputMethod();
        registry().push_back(this);
    }

    // RAII Native выполняет освобождение всех системных ресурсов; окно уходит из общего ожидания.
    // Native RAII releases all system resources; the window leaves the shared waiting.
    X11Window::~X11Window()
    {
        std::vector<X11Window*>& windows = registry();

        windows.erase(std::remove(windows.begin(), windows.end(), this), windows.end());
        destroySync();
    }

    // Input method использует локаль пользователя и возвращает UTF-8, в том числе кириллицу.
    // The input method uses the user locale and returns UTF-8, including Cyrillic.
    void X11Window::inputMethod()
    {
        std::setlocale(LC_CTYPE, "");
        XSetLocaleModifiers("");
        _native.method = XOpenIM(_native.display, nullptr, nullptr, nullptr);
        if (_native.method == nullptr) return;
        _native.input = XCreateIC(_native.method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
            XNClientWindow, _native.window, XNFocusWindow, _native.window, nullptr);
    }
}
