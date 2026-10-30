#include "gran/input/input_router.hpp"

namespace gran::input
{
    /*
    ** Кнопка команды или меню (command, popup) не забирает фокус у поля ввода — как панели инструментов
    ** и строка меню настольных программ: «Отменить», «Вставить» и меню «Правка» работают с этим полем.
    ** Tab переводит фокус на такую кнопку как обычно.
    **
    ** A command or menu button (command, popup) does not take the focus from an input — as toolbars
    ** and the menu bar of desktop programs: "Undo", "Paste" and the "Edit" menu work with this field.
    ** Tab moves the focus to such a button as usual.
    */
    static bool keepsEditing(const RouterState& state, const ui::Node* control) noexcept
    {
        if (control == nullptr || control->kind != ui::ElementKind::BUTTON)
            return false;
        if (state.focus == nullptr || !ui::isTextField(*state.focus))
            return false;
        return !control->content.command.empty() || !control->content.popup.empty();
    }

    /*
    ** Нажатие левой кнопкой: элемент управления становится нажатым и получает фокус (кроме keepsEditing; нажатие мимо
    ** элементов снимает фокус), ползунок начинает перетаскивание, в поле ввода и Text начинается выделение
    ** (нажатие мимо текста его снимает). У перетаскиваемого узла текст не выделяется: мышь тянет узел.
    **
    ** A left button press: the control becomes pressed and gets the focus (except keepsEditing; a press outside the controls
    ** removes the focus), a slider starts dragging, in an input and a Text a selection starts
    ** (a press outside the text removes it). A draggable node text is not selected: the mouse drags the node.
    */
    void pressLeft(RouterState& state, ui::Node* hit, const platform::Event& event)
    {
        ui::Node* control = controlOf(hit);
        ui::Node* text = nullptr;

        if (control != nullptr && disabled(control))
            control = nullptr;
        bool keep = keepsEditing(state, control);
        ui::Node* draggable = draggableOf(hit, control);
        if (draggable == nullptr)
            text = textTarget(hit, control);
        // Нажатие в меню выделение не снимает: команда «Копировать» берёт именно его
        // A press in a menu does not drop the selection: the "Copy" command takes exactly it
        if (text != nullptr)
            beginTextSelection(state, *text, event);
        else if (ui::popupOf(hit) == nullptr && !keep)
            clearSelection(state);
        changePressed(state, control);
        if (!keep)
            changeFocus(state, control);
        armDrag(state, draggable, event.position);
        if (control != nullptr && control->kind == ui::ElementKind::SLIDER)
        {
            state.drag = control;
            dragSlider(state, *control, ui::windowToNode(*control, event.position).x);
        }
    }

    /*
    ** Отпускание над тем же элементом — щелчок (кроме ползунка: его значение уже менялось). Второе отпускание
    ** двойного щелчка — doubleClick узлу под мышью, если щелчок не удалил узлы. allowed == false (pointerUp
    ** отменён, шло перетаскивание узла) — только сброс нажатия.
    **
    ** A release over the same control is a click (except a slider: its value was already changing). The second release
    ** of a double click is doubleClick to the node under the mouse if the click did not remove nodes. allowed == false (pointerUp
    ** is cancelled, a node drag was going on) — only resetting the press.
    */
    void releaseLeft(RouterState& state, ui::Node* hit, const platform::Event& event, bool allowed)
    {
        ui::Node* pressed = state.pressed;
        std::uint32_t version = state.actions->treeVersion();

        changePressed(state, nullptr);
        state.drag = nullptr;
        state.selecting = nullptr;
        state.nodeDrag.source = nullptr;
        if (!allowed)
            return;
        if (pressed != nullptr && pressed == controlOf(hit) && pressed->kind != ui::ElementKind::SLIDER)
            activate(state, *pressed);
        if (state.downClicks >= 2 && state.actions->treeVersion() == version)
            firePointer(state, ui::EventType::DOUBLE_CLICK, hit, event);
    }

    // Данные по умолчанию: формат dragFormat (или "node"), данные dragData (или id); dragStart может их сменить
    // The default data: the dragFormat format (or "node"), the dragData data (or the id); dragStart may change them
    void armDrag(RouterState& state, ui::Node* draggable, Vec2 point)
    {
        NodeDrag& drag = state.nodeDrag;

        drag = NodeDrag{};
        if (draggable == nullptr)
            return;
        drag.source = draggable;
        drag.origin = point;
        drag.data.format = draggable->content.dragFormat;
        drag.data.data = draggable->content.dragData;
        if (drag.data.format.empty())
            drag.data.format = "node";
        if (drag.data.data.empty())
            drag.data.data = draggable->id;
    }
}
