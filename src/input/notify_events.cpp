#include "gran/input/input_router.hpp"

#include <vector>

namespace gran::input
{
    namespace
    {
        // Уведомление узлу node; related — второй участник смены (откуда или куда) / A notification to the node; related is the second participant of the change (from where or to where)
        ui::UiEvent relatedEvent(ui::EventType type, ui::Node& node, ui::Node* related, Vec2 point)
        {
            ui::UiEvent event = ui::makeEvent(type, node);

            event.related = related;
            event.position = point;
            return event;
        }
    }

    /*
    ** Как pointerenter/pointerleave в DOM: событие получает каждый узел, который вошёл в цепочку под мышью
    ** или вышел из неё (общие предки — нет). Уход — от глубокого к внешнему, приход — от внешнего к глубокому.
    **
    ** As pointerenter/pointerleave in the DOM: every node that entered the chain under the mouse or left it
    ** gets the event (the common ancestors do not). Leaving — from the deep to the outer one, entering — from the outer to the deep one.
    */
    void notifyHover(RouterState& state, ui::Node* from, ui::Node* to)
    {
        std::vector<ui::Node*> entered;

        for (ui::Node* node = from; node != nullptr; node = node->parent)
        {
            if (!within(to, *node))
                queueEvent(state, relatedEvent(ui::EventType::POINTER_LEAVE, *node, to, state.pointer));
        }
        for (ui::Node* node = to; node != nullptr; node = node->parent)
        {
            if (!within(from, *node))
                entered.push_back(node);
        }
        for (std::size_t index = entered.size(); index > 0; --index)
            queueEvent(state, relatedEvent(ui::EventType::POINTER_ENTER, *entered[index - 1], from, state.pointer));
    }

    // Сначала blur ушедшему, затем focus пришедшему — как в браузерах / First blur to the one left, then focus to the one come — as in browsers
    void notifyFocus(RouterState& state, ui::Node* from, ui::Node* to)
    {
        if (from != nullptr)
            queueEvent(state, relatedEvent(ui::EventType::BLUR, *from, to, state.pointer));
        if (to != nullptr)
            queueEvent(state, relatedEvent(ui::EventType::FOCUS, *to, from, state.pointer));
    }
}
