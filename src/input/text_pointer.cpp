#include "gran/input/internal/text_cursor.hpp"
#include "gran/input/internal/text_history.hpp"

namespace gran::input
{
    // Поле ввода выделяется всегда; Text — если он не часть элемента управления (подпись кнопки нажимает кнопку)
    // An input is always selectable; a Text — if it is not part of a control (a button label presses the button)
    ui::Node* textTarget(ui::Node* hit, ui::Node* control) noexcept
    {
        if (control != nullptr && ui::isTextField(*control))
            return control;
        if (control == nullptr && hit != nullptr && hit->kind == ui::ElementKind::TEXT && !disabled(hit))
            return hit;
        return nullptr;
    }

    /*
    ** Нажатие в тексте. Место считается до смены фокуса: прокрутка длинного поля зависит
    ** от каретки, а пользователь щёлкнул по тому, что видел. Новый узел снимает выделение
    ** со старого; Shift продлевает выделение только в том же узле.
    **
    ** A press in the text. The place is computed before the focus changes: the scroll of a long input
    ** depends on the caret, and the user clicked what they saw. A new node removes the selection
    ** from the old one; Shift extends the selection only in the same node.
    */
    void beginTextSelection(RouterState& state, ui::Node& node, const platform::Event& event)
    {
        if (state.text == nullptr)
            return;
        std::size_t offset = state.text->offsetAt(node, event.position);
        bool extend = (event.modifiers & platform::MODIFIER_SHIFT) != 0 && state.selection == &node;

        if (state.selection != &node)
            clearSelection(state);
        state.selection = &node;
        state.selecting = &node;
        if (event.clicks >= 2)
            internal::selectWord(node, offset);
        else
            internal::placeCaret(node, offset, extend);
    }

    // Якорь остаётся, каретка — в точку под мышью / The anchor stays, the caret goes to the point under the mouse
    void dragTextSelection(RouterState& state, Vec2 point)
    {
        if (state.selecting == nullptr || state.text == nullptr)
            return;
        internal::placeCaret(*state.selecting, state.text->offsetAt(*state.selecting, point), true);
    }

    // Инспектор временно забирает ввод, сохраняя диапазон редактируемого текста.
    // The layout inspector temporarily takes input while preserving the edited text range.
    void clearSelection(RouterState& state, bool preserveRange)
    {
        if (state.selection == nullptr)
            return;
        if (preserveRange)
            internal::breakTyping(*state.selection);
        else
            internal::placeCaret(*state.selection, state.selection->content.caret, false);
        state.selection = nullptr;
        state.selecting = nullptr;
    }

    // Ctrl+A и фокус клавишей Tab: якорь в начале, каретка в конце / Ctrl+A and the Tab focus: the anchor at the start, the caret at the end
    void selectAll(RouterState& state, ui::Node& node)
    {
        if (state.selection != &node)
            clearSelection(state);
        state.selection = &node;
        node.content.anchor = 0;
        internal::placeCaret(node, node.content.text.size(), true);
    }
}
