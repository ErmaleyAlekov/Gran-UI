#include "gran/input/input_router.hpp"

#include <cctype>

namespace gran::input
{
    namespace
    {
        // Список форматов acceptDrop (через пробелы) содержит format или «*» / The acceptDrop format list (by spaces) contains format or "*"
        bool acceptsFormat(const std::string& formats, const std::string& format) noexcept
        {
            std::size_t start = 0;

            while (start < formats.size())
            {
                std::size_t end = start;
                while (end < formats.size() && std::isspace(static_cast<unsigned char>(formats[end])) == 0)
                    ++end;
                std::size_t length = end - start;
                if (length > 0 && (formats.compare(start, length, "*") == 0 || formats.compare(start, length, format) == 0))
                    return true;
                start = end + 1;
            }
            return false;
        }
    }

    /*
    ** Внутри перетаскиваемой карточки поле ввода или ползунок остаются собой: элемент управления глубже
    ** перетаскиваемого узла отменяет перетаскивание. Кнопка, которая сама draggable, — перетаскивается.
    **
    ** Inside a draggable card an input or a slider stay themselves: a control deeper than
    ** the draggable node cancels the drag. A button that is draggable itself is dragged.
    */
    ui::Node* draggableOf(ui::Node* hit, ui::Node* control) noexcept
    {
        for (ui::Node* node = hit; node != nullptr; node = node->parent)
        {
            if (!node->content.draggable)
                continue;
            if (disabled(node) || (control != nullptr && control != node && within(control, *node)))
                return nullptr;
            return node;
        }
        return nullptr;
    }

    // Узел сам в себя не бросается: поддерево источника зоной не бывает / A node is not dropped into itself: the source subtree is never a zone
    ui::Node* dropZoneAt(RouterState& state, Vec2 point)
    {
        const NodeDrag& drag = state.nodeDrag;

        for (ui::Node* node = pickNode(state, point); node != nullptr; node = node->parent)
        {
            if (drag.source != nullptr && within(node, *drag.source))
                continue;
            if (!node->content.acceptDrop.empty() && acceptsFormat(node->content.acceptDrop, drag.data.format))
            {
                if (disabled(node))
                    return nullptr;
                return node;
            }
        }
        return nullptr;
    }

    /*
    ** Состояние DropTarget снимается со старой зоны сразу; новая получает его по dragOver (если зона согласна).
    ** Обработчик dragLeave мог удалить новую зону — тогда dragEnter ей не идёт (ссылку обнулил forget).
    **
    ** The DropTarget state is removed from the old zone at once; the new one gets it by dragOver (if the zone agrees).
    ** A dragLeave handler might remove the new zone — then dragEnter does not go to it (forget cleared the reference).
    */
    void changeZone(RouterState& state, ui::Node* zone, const platform::Event& source, bool notify)
    {
        NodeDrag& drag = state.nodeDrag;
        ui::Node* old = drag.zone;

        if (old == zone)
            return;
        drag.zone = zone;
        drag.allowed = false;
        if (old != nullptr)
        {
            setNodeState(*old, style::stateBit(style::State::DROP_TARGET), false);
            ui::UiEvent leave = dragEvent(state, ui::EventType::DRAG_LEAVE, *old, source);
            if (notify)
                fire(state, leave);
        }
        if (zone != nullptr && notify && drag.zone == zone)
        {
            ui::UiEvent enter = dragEvent(state, ui::EventType::DRAG_ENTER, *zone, source);
            fire(state, enter);
        }
    }

    // У событий источника второй участник — зона, у событий зоны — источник / For the source events the second participant is the zone, for the zone events — the source
    ui::UiEvent dragEvent(const RouterState& state, ui::EventType type, ui::Node& node, const platform::Event& source)
    {
        ui::UiEvent event = pointerEvent(type, node, source);

        event.drag = state.nodeDrag.data;
        event.related = state.nodeDrag.source;
        if (&node == state.nodeDrag.source)
            event.related = state.nodeDrag.zone;
        return event;
    }
}
