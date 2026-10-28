#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Интерактивный потомок заголовка оставляет клиентский ввод; текст наследует drag.
    // An interactive title bar child keeps client input; text inherits drag.
    platform::WindowRole InputRouter::windowRoleAt(Vec2 point)
    {
        ui::Node* hit = pickNode(_state, point);

        if (disabled(hit))
            return platform::WindowRole::NONE;
        for (ui::Node* node = hit; node != nullptr; node = node->parent)
        {
            if (node->content.windowRole != platform::WindowRole::NONE)
                return node->content.windowRole;
            if (controlOf(node) == node || node->kind == ui::ElementKind::WEB_VIEW || ui::isLayer(*node))
                break;
        }
        return platform::WindowRole::NONE;
    }

    // Развернуть превращается в восстановить, когда окно уже развёрнуто системой.
    // Maximize becomes restore when the system has already maximized the window.
    void activateWindow(RouterState& state, const ui::Node& node)
    {
        if (state.window == nullptr || disabled(&node))
            return;
        switch (node.content.windowRole)
        {
            case platform::WindowRole::MINIMIZE: state.window->minimize(); break;
            case platform::WindowRole::CLOSE: state.window->close(); break;
            case platform::WindowRole::MAXIMIZE:
                if (state.window->isMaximized())
                    state.window->restore();
                else
                    state.window->maximize();
                break;
            default: break;
        }
    }
}
