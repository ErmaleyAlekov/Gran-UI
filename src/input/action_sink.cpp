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

    // Получатель без команд не знает сочетаний: клавиша идёт встроенной обработке / A receiver without commands knows no shortcuts: the key goes to the built-in handling
    bool ActionSink::shortcut(const CommandQuery&)
    {
        return false;
    }

    // Получатель без команд пропускает ссылку, как действие без обработчика / A receiver without commands skips the reference, as an action without a handler
    void ActionSink::command(const std::string&, ui::Node&)
    {
    }

    // Получатель без команд отмены не следит за полями / A receiver without undo commands does not watch the fields
    void ActionSink::editingChanged()
    {
    }
}
