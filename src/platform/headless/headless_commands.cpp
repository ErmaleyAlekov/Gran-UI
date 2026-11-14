#include "gran/platform/headless/headless_window.hpp"

namespace gran::platform
{
    void HeadlessWindow::minimize()
    {
        _state.minimized = true;
    }

    void HeadlessWindow::maximize()
    {
        _state.minimized = false;
        _state.maximized = true;
    }

    // Восстановить: из свёрнутого — к прежнему виду, из развёрнутого — к обычному
    // Restore: from minimized — to the former look, from maximized — to normal
    void HeadlessWindow::restore()
    {
        if (_state.minimized)
            _state.minimized = false;
        else
            _state.maximized = false;
    }

    // Как у ОС: окно не закрывается сразу, а присылает CLOSE — решает приложение
    // As the OS does: the window does not close at once but sends CLOSE — the application decides
    void HeadlessWindow::close()
    {
        Event event;

        event.kind = EventKind::CLOSE;
        pushEvent(event);
    }

    bool HeadlessWindow::isMaximized() const
    {
        return _state.maximized;
    }
}
