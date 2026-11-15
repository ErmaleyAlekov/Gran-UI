#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>

namespace gran::platform
{
    // Фабрика окна macOS / The macOS window factory
    std::unique_ptr<Window> createWindow(const WindowDesc& desc)
    {
        return std::make_unique<cocoa::CocoaWindow>(desc);
    }
}

namespace gran::platform::cocoa
{
    /*
    ** AppKit требует главный поток: окно из другого потока — понятная ошибка, а не падение внутри AppKit.
    ** Окно скрыто до первого кадра (show), как в Windows и X11.
    **
    ** AppKit requires the main thread: a window from another thread is a clear error, not a crash inside AppKit.
    ** The window is hidden until the first frame (show), as on Windows and X11.
    */
    CocoaWindow::CocoaWindow(const WindowDesc& desc)
    {
        if (![NSThread isMainThread])
            throw GranException(ErrorCode::PLATFORM,
                i18n::format(i18n::Message::PLATFORM_WINDOW_FAILED, {"AppKit: not the main thread"}));
        @autoreleasepool
        {
            ensureApplication();
            create(desc);
            configure(desc);
            place(desc);
        }
        _pixels = pixelSize();
        registry().push_back(this);
    }

    // Связи Objective-C → C++ рвутся до освобождения объектов: позднее событие AppKit найдёт owner == nullptr
    // The Objective-C → C++ links are broken before the objects are released: a late AppKit event finds owner == nullptr
    CocoaWindow::~CocoaWindow()
    {
        std::vector<CocoaWindow*>& windows = registry();

        windows.erase(std::remove(windows.begin(), windows.end(), this), windows.end());
        @autoreleasepool
        {
            _view.owner = nullptr;
            _window.owner = nullptr;
            _delegate.owner = nullptr;
            _window.delegate = nil;
            if (_window.parentWindow != nil)
                [_window.parentWindow removeChildWindow:_window];
            [_window orderOut:nil];
            [_window close];
        }
    }

    // Только поток интерфейса создаёт и уничтожает окна, поэтому список без замка
    // Only the UI thread creates and destroys windows, so the list needs no lock
    std::vector<CocoaWindow*>& CocoaWindow::registry()
    {
        static std::vector<CocoaWindow*> windows;

        return windows;
    }
}
