#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Элемент управления: встроенный вид или любой узел с onClick / A control: a built-in kind or any node with onClick
        bool isControl(const ui::Node& node) noexcept
        {
            ui::ElementKind kind = node.kind;

            return kind == ui::ElementKind::BUTTON || kind == ui::ElementKind::CHECKBOX || kind == ui::ElementKind::SWITCH
                || kind == ui::ElementKind::RADIO_BUTTON || kind == ui::ElementKind::TAB_HEADER
                || kind == ui::ElementKind::SLIDER || ui::isTextField(node)
                || kind == ui::ElementKind::MENU_ITEM || kind == ui::ElementKind::COMBO_BOX
                || !node.content.onClick.empty() || ui::isWindowButton(node);
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
    ** Активация: пункт меню закрывает свой слой и выполняет команду (пункт с подменю открывает его);
    ** флажок и переключатель переключаются, радиокнопка и вкладка выбираются (CHECKED и onChange);
    ** кнопка с popup и ComboBox открывают свой слой;
    ** затем выполняется onClick. Состояния узлов меняются до вызова приложения — его обработчик видит новое значение.
    **
    ** Activation: a menu item closes its layer and runs its command (an item with a submenu opens it);
    ** a check box and a switch toggle, a radio button and a tab get chosen (CHECKED and onChange);
    ** a button with popup and a ComboBox open their layer;
    ** then onClick runs. The node states change before calling the application — its handler sees the new value.
    */
    void activate(RouterState& state, ui::Node& control)
    {
        if (disabled(&control))
            return;
        activateWindow(state, control);
        if (control.kind == ui::ElementKind::MENU_ITEM && ui::submenuOf(control) != nullptr)
            openSubmenu(state, control);
        else if (control.kind == ui::ElementKind::MENU_ITEM)
            chooseItem(state, control);
        if (control.kind == ui::ElementKind::MENU_ITEM)
            return;
        openAttached(state, control);
        changeChoice(state, control);
        if (!control.content.onClick.empty())
            state.actions->action(control.content.onClick, control);
    }
}
