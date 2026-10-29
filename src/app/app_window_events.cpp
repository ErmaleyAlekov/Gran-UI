#include "gran/app/internal/app_state.hpp"

namespace gran
{
    // Копия обработчика: он может заменить сам себя через onEvent / A handler copy: it may replace itself through onEvent
    void app::AppWindow::event(const std::string& name, ui::Node& node, ui::UiEvent& event)
    {
        auto handler = shared.eventHandlers.find(name);

        if (handler == shared.eventHandlers.end() || !handler->second)
        {
            action(name, node);
            return;
        }
        EventHandler callback = handler->second;
        callback(node, event);
    }

    // Удаления в любом окне считаются: обработчик одного окна может править другое / Removals in any window count: a handler of one window may edit another
    std::uint32_t app::AppWindow::treeVersion() const noexcept
    {
        return shared.treeVersion;
    }
}
