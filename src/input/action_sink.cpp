#include "gran/input/input_router.hpp"

namespace gran::input
{
    // Без своей логики событие из разметки — обычное действие: onKeyDown="gran:close" работает как onClick
    // Without its own logic a markup event is a plain action: onKeyDown="gran:close" works as onClick does
    void ActionSink::event(const std::string& name, ui::Node& node, ui::UiEvent&)
    {
        action(name, node);
    }

    // Получатель, не знающий об удалениях, считает дерево неизменным / A receiver that knows nothing of removals treats the tree as unchanged
    std::uint32_t ActionSink::treeVersion() const noexcept
    {
        return 0;
    }
}
