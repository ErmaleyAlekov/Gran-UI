#include "gran/app/internal/app_state.hpp"

#include <utility>

namespace gran
{
    // Обработчик заменяет прежний с тем же именем / The handler replaces a former one with the same name
    void Application::onFileDrop(const std::string& name, FileDropHandler handler)
    {
        _state->dropHandlers[name] = std::move(handler);
    }

    namespace app
    {
        namespace
        {
            // Ближайшая к точке включённая зона onDrop (узел под точкой или его предок) / The enabled onDrop zone nearest to the point (the node under the point or its ancestor)
            ui::Node* dropZoneAt(AppWindow& win, Vec2 point)
            {
                ui::Node* node = input::hitTest(*win.document.root, point);

                while (node != nullptr && (node->content.onDrop.empty() || !node->enabled))
                    node = node->parent;
                return node;
            }

            /*
            ** Состояние DropTarget — только у самой зоны (не у предков, как hover): вложенные зоны
            ** не подсвечивают внешнюю. true — подсветка сменилась.
            **
            ** The DropTarget state is only on the zone itself (not on the ancestors, as hover): nested zones
            ** do not highlight the outer one. true — the highlight changed.
            */
            bool markDropTarget(AppWindow& win, ui::Node* node)
            {
                std::uint16_t bit = style::stateBit(style::State::DROP_TARGET);

                if (win.dropNode == node)
                    return false;
                if (win.dropNode != nullptr)
                {
                    win.dropNode->stateMask = static_cast<std::uint16_t>(win.dropNode->stateMask & ~bit);
                    ui::markDirty(*win.dropNode, ui::DIRTY_STYLE | ui::DIRTY_PAINT);
                }
                win.dropNode = node;
                if (node != nullptr)
                {
                    node->stateMask = static_cast<std::uint16_t>(node->stateMask | bit);
                    ui::markDirty(*node, ui::DIRTY_STYLE | ui::DIRTY_PAINT);
                }
                return true;
            }

            // Пути забираются у окна всегда (очередь путей идёт парой с событиями DROP) / The paths are always taken from the window (the path queue goes in pairs with the DROP events)
            void deliverDrop(AppWindow& win, ui::Node* zone)
            {
                std::vector<std::string> files = win.window->takeDroppedFiles();

                if (zone == nullptr || files.empty())
                    return;
                auto handler = win.shared.dropHandlers.find(zone->content.onDrop);
                if (handler != win.shared.dropHandlers.end() && handler->second)
                    handler->second(*zone, files);
            }
        }

        /*
        ** DRAG_MOVE — подсветить зону под точкой (или снять), DRAG_LEAVE — снять, DROP — снять
        ** и отдать пути обработчику зоны.
        **
        ** DRAG_MOVE — highlight the zone under the point (or remove it), DRAG_LEAVE — remove it, DROP — remove it
        ** and give the paths to the zone handler.
        */
        bool handleDrag(AppWindow& win, const platform::Event& event)
        {
            ui::Node* zone = nullptr;
            bool changed = false;

            if (event.kind != platform::EventKind::DRAG_LEAVE)
                zone = dropZoneAt(win, event.position);
            if (event.kind == platform::EventKind::DRAG_MOVE)
                changed = markDropTarget(win, zone);
            else
                changed = markDropTarget(win, nullptr);
            if (event.kind == platform::EventKind::DROP)
            {
                deliverDrop(win, zone);
                changed = true;
            }
            return changed;
        }
    }
}
