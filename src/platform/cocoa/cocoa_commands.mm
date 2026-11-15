#include "gran/platform/cocoa/internal/cocoa_window_impl.hpp"

namespace gran::platform::cocoa
{
    // Свернуть в Dock / Minimize to the Dock
    void CocoaWindow::minimize()
    {
        [_window miniaturize:nil];
    }

    /*
    ** «Развернуть» macOS — zoom: окно занимает экран без строки меню и Dock (не полноэкранный режим). zoom:
    ** переключает состояние, поэтому вызывается только если окно ещё не развёрнуто.
    **
    ** The macOS "maximize" is zoom: the window takes the screen without the menu bar and the Dock (not the full screen
    ** mode). zoom: toggles the state, so it is called only when the window is not maximized yet.
    */
    void CocoaWindow::maximize()
    {
        if (!_window.zoomed)
            [_window zoom:nil];
    }

    // Восстановить: из Dock, иначе из развёрнутого / Restore: from the Dock, otherwise from maximized
    void CocoaWindow::restore()
    {
        if (_window.miniaturized)
            [_window deminiaturize:nil];
        else if (_window.zoomed)
            [_window zoom:nil];
    }

    // Закрытие — обычным событием CLOSE: программа решает сама, разрушать ли окно (контракт платформы)
    // Closing is the usual CLOSE event: the program decides itself whether to destroy the window (the platform contract)
    void CocoaWindow::close()
    {
        push(EventKind::CLOSE);
    }

    bool CocoaWindow::isMaximized() const
    {
        return _window.zoomed;
    }
}
