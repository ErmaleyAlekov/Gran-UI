#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        /*
        ** Подходит ли обработчик (из разметки или кода — поля одинаковые): вид, фаза и, у своего события, имя.
        ** Шаблон, а не две копии условия: EventBinding и EventListener различаются только способом вызова.
        **
        ** Whether a handler fits (from the markup or code — the fields are the same): the kind, the phase and, for a custom event, the name.
        ** A template, not two copies of the condition: EventBinding and EventListener differ only in the way of calling.
        */
        template <typename Entry>
        bool matches(const Entry& entry, const ui::UiEvent& event, bool capture) noexcept
        {
            if (entry.type != event.type || entry.capture != capture)
                return false;
            return event.type != ui::EventType::CUSTOM || entry.name == event.name;
        }

        /*
        ** Обработчики разметки узла. Проверка immediateStopped — до любого обращения к узлу: если обработчик
        ** удалил узлы (версия дерева сменилась), node мог повиснуть, и доставка обрывается.
        **
        ** The node markup handlers. The immediateStopped check comes before any access to the node: if a handler
        ** removed nodes (the tree version changed), node might dangle, and the delivery breaks off.
        */
        void invokeBindings(ui::Node& node, ui::UiEvent& event, ActionSink& sink, bool capture, std::uint32_t version)
        {
            for (std::size_t index = 0; !event.immediateStopped && index < node.content.events.size(); ++index)
            {
                if (!matches(node.content.events[index], event, capture))
                    continue;
                // Копия имени: обработчик может поменять разметку узла / A name copy: the handler may change the node markup
                std::string action = node.content.events[index].action;
                event.handled++;
                sink.event(action, node, event);
                if (sink.treeVersion() != version)
                    event.stopImmediatePropagation();
            }
        }

        // Слушатели кода; копии обработчиков — слушатель может снять себя или соседей / The code listeners; handler copies — a listener may remove itself or its neighbors
        void invokeListeners(ui::Node& node, ui::UiEvent& event, ActionSink& sink, bool capture, std::uint32_t version)
        {
            std::vector<ui::EventHandler> handlers;

            if (event.immediateStopped)
                return;
            for (const ui::EventListener& listener : node.listeners)
            {
                if (listener.handler && matches(listener, event, capture))
                    handlers.push_back(listener.handler);
            }
            for (ui::EventHandler& handler : handlers)
            {
                if (event.immediateStopped)
                    return;
                event.handled++;
                handler(node, event);
                if (sink.treeVersion() != version)
                    event.stopImmediatePropagation();
            }
        }

        // Узел пути: current, фаза, точка в его системе; на цели — сначала обработчики захвата, затем всплытия
        // A path node: current, the phase, the point in its system; at the target — the capture handlers first, then the bubbling ones
        void visit(ui::Node& node, ui::UiEvent& event, ActionSink& sink, ui::EventPhase phase, std::uint32_t version)
        {
            Vec2 point = ui::windowToNode(node, event.position);

            event.current = &node;
            event.phase = phase;
            event.local = Vec2{point.x - node.layout.frame.x, point.y - node.layout.frame.y};
            if (phase != ui::EventPhase::BUBBLE)
            {
                invokeBindings(node, event, sink, true, version);
                invokeListeners(node, event, sink, true, version);
            }
            if (phase != ui::EventPhase::CAPTURE)
            {
                invokeBindings(node, event, sink, false, version);
                invokeListeners(node, event, sink, false, version);
            }
        }
    }

    /*
    ** Путь собирается до доставки (как в DOM): узлы, вставленные обработчиками, его не меняют. Указатели пути
    ** не разыменовываются после остановки — так удаление узла обработчиком безопасно.
    **
    ** The path is gathered before the delivery (as in the DOM): the nodes inserted by handlers do not change it. The path pointers
    ** are not dereferenced after a stop — so removing a node by a handler is safe.
    */
    bool dispatchEvent(ui::UiEvent& event, ActionSink& sink)
    {
        std::vector<ui::Node*> path;
        std::uint32_t version = sink.treeVersion();

        if (event.target == nullptr)
            return true;
        for (ui::Node* node = event.target; node != nullptr; node = node->parent)
            path.push_back(node);
        for (std::size_t index = path.size(); index > 1 && !event.propagationStopped; --index)
            visit(*path[index - 1], event, sink, ui::EventPhase::CAPTURE, version);
        if (!event.propagationStopped)
            visit(*path[0], event, sink, ui::EventPhase::TARGET, version);
        for (std::size_t index = 1; index < path.size() && event.bubbles && !event.propagationStopped; ++index)
            visit(*path[index], event, sink, ui::EventPhase::BUBBLE, version);
        event.current = nullptr;
        event.phase = ui::EventPhase::NONE;
        return !event.defaultPrevented;
    }
}
