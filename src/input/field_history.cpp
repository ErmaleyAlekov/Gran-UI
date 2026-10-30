#include "gran/input/input_router.hpp"
#include "gran/input/internal/text_history.hpp"

namespace gran::input
{
    // Слои просматриваются сверху: фокус внутри меню заменяется узлом, куда он вернётся / The layers are viewed from the top: a focus inside a menu is replaced with the node it returns to
    ui::Node* editingField(const RouterState& state) noexcept
    {
        ui::Node* node = state.focus;

        for (std::size_t index = state.layers.size(); index > 0; --index)
        {
            const OpenLayer& layer = state.layers[index - 1];
            if (layer.node->kind == ui::ElementKind::DIALOG)
                break;
            if (node != nullptr && within(node, *layer.node))
                node = layer.returnFocus;
        }
        if (node == nullptr || !ui::isTextField(*node))
            return nullptr;
        return node;
    }

    bool InputRouter::canUndoField(bool redo)
    {
        ui::Node* field = editingField(_state);

        if (field == nullptr)
            return false;
        if (redo)
            return internal::canRedoText(*field);
        return internal::canUndoText(*field);
    }

    // Пункт меню уже закрыл меню и вернул фокус полю: правка идёт там же, где Ctrl+Z / The menu item already closed the menu and returned the focus to the field: the edit goes where Ctrl+Z would
    bool InputRouter::undoField(bool redo)
    {
        ui::Node* field = editingField(_state);

        if (field == nullptr)
            return false;
        if (redo)
            return internal::redoText(_state, *field);
        return internal::undoText(_state, *field);
    }
}
