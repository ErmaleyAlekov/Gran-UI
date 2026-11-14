#include "gran/platform/headless/headless_window.hpp"

namespace gran::platform
{
    // Дерево доступности хранится для теста: что приложение подключило / The accessibility tree is kept for a test: what the application connected
    void HeadlessWindow::setAccessTree(a11y::AccessTree* tree)
    {
        _state.accessTree = tree;
    }

    // Дикторов нет — уведомления считаются, чтобы тест видел, что они были
    // There are no screen readers — the notifications are counted so a test sees they happened
    void HeadlessWindow::accessFocusChanged()
    {
        ++_state.focusChanges;
    }

    // Сверка состояния нужна только провайдеру ОС; здесь сравнивать нечего
    // The state comparison is needed only by an OS provider; there is nothing to compare here
    void HeadlessWindow::accessRefresh()
    {
    }

    void HeadlessWindow::accessAnnounce(const ui::Node&)
    {
        ++_state.announcements;
    }

    const HeadlessState& HeadlessWindow::state() const noexcept
    {
        return _state;
    }
}
