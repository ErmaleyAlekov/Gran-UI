#include "gran/input/input_router.hpp"

#include <utility>

namespace gran::input
{
    // Удаление узлов обработчиком отменяет действие по умолчанию: узлы, с которыми оно работало бы, могли исчезнуть
    // Removing nodes by a handler cancels the default action: the nodes it would work with might be gone
    bool fire(RouterState& state, ui::UiEvent& event)
    {
        std::uint32_t version = state.actions->treeVersion();
        bool allowed = dispatchEvent(event, *state.actions);

        state.delivered += event.handled;
        return allowed && state.actions->treeVersion() == version;
    }

    // Порядок очереди — порядок изменений / The queue order is the order of the changes
    void queueEvent(RouterState& state, ui::UiEvent event)
    {
        state.pending.push_back(std::move(event));
    }

    /*
    ** Событие снимается с очереди до доставки: обработчик может сам поменять фокус или наведение
    ** (новые уведомления встанут в конец) и даже вызвать доставку вложенно — очередь всё равно опустеет.
    **
    ** An event is taken off the queue before the delivery: a handler may change the focus or the hover itself
    ** (the new notifications go to the end) and even call the delivery nestedly — the queue empties anyway.
    */
    void flushEvents(RouterState& state)
    {
        while (!state.pending.empty())
        {
            ui::UiEvent event = std::move(state.pending.front());

            state.pending.erase(state.pending.begin());
            dispatchEvent(event, *state.actions);
            state.delivered += event.handled;
        }
    }

    // Уведомление о самом удаляемом узле пропадает, о соседе — теряет только ссылку related
    // A notification about the removed node itself is lost, about a neighbor — it loses only the related reference
    void forgetEvents(RouterState& state, const ui::Node& subtree) noexcept
    {
        for (std::size_t index = state.pending.size(); index > 0; --index)
        {
            ui::UiEvent& event = state.pending[index - 1];

            if (within(event.related, subtree))
                event.related = nullptr;
            if (within(event.target, subtree))
                state.pending.erase(state.pending.begin() + static_cast<std::ptrdiff_t>(index - 1));
        }
    }
}
