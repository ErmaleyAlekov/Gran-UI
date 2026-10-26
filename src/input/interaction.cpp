#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Элемент управления: встроенный вид или любой узел с onClick / A control: a built-in kind or any node with onClick
        bool isControl(const ui::Node& node) noexcept
        {
            return node.kind == ui::ElementKind::BUTTON || node.kind == ui::ElementKind::CHECKBOX
                || node.kind == ui::ElementKind::SLIDER || ui::isTextField(node)
                || node.kind == ui::ElementKind::MENU_ITEM
                || !node.content.onClick.empty();
        }
    }

    // Подъём к ближайшему элементу управления (текст внутри кнопки нажимает кнопку)
    // Climbing to the nearest control (a text inside a button presses the button)
    ui::Node* controlOf(ui::Node* node) noexcept
    {
        for (ui::Node* current = node; current != nullptr; current = current->parent)
        {
            if (isControl(*current))
                return current;
        }
        return nullptr;
    }

    // Отключённый предок отключает всё поддерево / A disabled ancestor disables the whole subtree
    bool disabled(const ui::Node* node) noexcept
    {
        for (const ui::Node* current = node; current != nullptr; current = current->parent)
        {
            if (!current->enabled)
                return true;
        }
        return false;
    }

    // Ближайшая прокрутка (Scroll или переполненное TextArea) / The nearest scroll (a Scroll or an overflowing TextArea)
    ui::Node* scrollOf(ui::Node* node) noexcept
    {
        for (ui::Node* current = node; current != nullptr; current = current->parent)
        {
            if (current->kind == ui::ElementKind::SCROLL)
                return current;
            // Многострочное поле прокручивает свой текст, если он выше поля (contentSize — от компоновки)
            // A multi-line input scrolls its own text if the text is taller than the field (contentSize comes from layout)
            if (current->kind == ui::ElementKind::TEXT_AREA
                && current->layout.contentSize.height > current->layout.frame.height - current->computed.borderWidth.top
                    - current->computed.borderWidth.bottom + 0.5f)
                return current;
        }
        return nullptr;
    }

    /*
    ** Активация: пункт меню закрывает свой слой и выполняет команду; флажок переключается (состояние
    ** CHECKED и onChange), затем выполняется onClick.
    ** Состояния узлов меняются до вызова приложения — его обработчик видит новое значение.
    **
    ** Activation: a menu item closes its layer and runs its command; a check box toggles (the CHECKED state
    ** and onChange), then onClick runs.
    ** The node states change before calling the application — its handler sees the new value.
    */
    void activate(RouterState& state, ui::Node& control)
    {
        if (control.kind == ui::ElementKind::MENU_ITEM)
        {
            chooseItem(state, control);
            return;
        }
        if (control.kind == ui::ElementKind::CHECKBOX)
        {
            control.content.checked = !control.content.checked;
            control.stateMask = static_cast<std::uint8_t>(control.stateMask ^ style::stateBit(style::State::CHECKED));
            ui::markDirty(control, ui::DIRTY_STYLE | ui::DIRTY_PAINT);
            if (!control.content.onChange.empty())
                state.actions->action(control.content.onChange, control);
        }
        if (!control.content.onClick.empty())
            state.actions->action(control.content.onClick, control);
    }
}
