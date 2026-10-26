#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Лежит ли узел в поддереве (подъём по родителям) / Whether the node lies in the subtree (climbing the parents)
        bool inside(const ui::Node* node, const ui::Node& subtree) noexcept
        {
            for (const ui::Node* current = node; current != nullptr; current = current->parent)
            {
                if (current == &subtree)
                    return true;
            }
            return false;
        }

        // Ссылка на узел поддерева обнуляется, остальные не трогаются / A reference to a subtree node is cleared, the others are left alone
        void drop(ui::Node*& reference, const ui::Node& subtree) noexcept
        {
            if (inside(reference, subtree))
                reference = nullptr;
        }
    }

    /*
    ** Как фокус щелчком, но без размещения каретки по мыши: прежнее выделение схлопывается,
    ** поле ввода становится узлом выделения (каретка видна, Shift+стрелки работают сразу).
    **
    ** As the focus by a click, but without placing the caret by the mouse: the former selection collapses,
    ** the input becomes the selection node (the caret is visible, Shift+arrows work at once).
    */
    void InputRouter::focus(ui::Node* node)
    {
        if (_state.selection != node)
            clearSelection(_state);
        changeFocus(_state, node);
        if (node != nullptr && ui::isTextField(*node))
            _state.selection = node;
    }

    /*
    ** Указатели маршрутизатора невладеющие: после удаления поддерева они повисли бы.
    ** Состояния (hover, pressed) снимать не нужно — узлы уходят вместе с ними.
    **
    ** The router pointers are non-owning: after the subtree removal they would dangle.
    ** The states (hover, pressed) need not be cleared — the nodes go away together with them.
    */
    void InputRouter::forget(const ui::Node& subtree) noexcept
    {
        drop(_state.hover, subtree);
        drop(_state.pressed, subtree);
        drop(_state.focus, subtree);
        drop(_state.drag, subtree);
        drop(_state.selection, subtree);
        drop(_state.selecting, subtree);
        drop(_state.returnFocus, subtree);
        drop(_state.contextMenu, subtree);
        drop(_state.contextTarget, subtree);
        for (std::size_t index = _state.popups.size(); index > 0; --index)
        {
            if (inside(_state.popups[index - 1], subtree))
                _state.popups.erase(_state.popups.begin() + static_cast<std::ptrdiff_t>(index - 1));
        }
    }
}
