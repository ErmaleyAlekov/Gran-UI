#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Ссылка на узел поддерева обнуляется, остальные не трогаются / A reference to a subtree node is cleared, the others are left alone
        void drop(ui::Node*& reference, const ui::Node& subtree) noexcept
        {
            if (within(reference, subtree))
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
        flushEvents(_state);
    }

    /*
    ** Указатели маршрутизатора невладеющие: после удаления поддерева они повисли бы.
    ** Состояния (hover, pressed) снимать не нужно — узлы уходят вместе с ними; отложенные уведомления
    ** об этих узлах отбрасываются.
    **
    ** The router pointers are non-owning: after the subtree removal they would dangle.
    ** The states (hover, pressed) need not be cleared — the nodes go away together with them; the deferred notifications
    ** about these nodes are dropped.
    */
    void InputRouter::forget(const ui::Node& subtree) noexcept
    {
        drop(_state.hover, subtree);
        drop(_state.pressed, subtree);
        drop(_state.focus, subtree);
        drop(_state.drag, subtree);
        drop(_state.selection, subtree);
        drop(_state.selecting, subtree);
        drop(_state.contextMenu, subtree);
        drop(_state.contextTarget, subtree);
        drop(_state.capture, subtree);
        drop(_state.nodeDrag.source, subtree);
        drop(_state.nodeDrag.zone, subtree);
        forgetEvents(_state, subtree);
        for (std::size_t index = _state.layers.size(); index > 0; --index)
        {
            drop(_state.layers[index - 1].returnFocus, subtree);
            if (within(_state.layers[index - 1].node, subtree))
                _state.layers.erase(_state.layers.begin() + static_cast<std::ptrdiff_t>(index - 1));
        }
    }
}
