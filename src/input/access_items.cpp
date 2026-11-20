#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        /*
        ** Как щелчок мышью: фокус уходит списку строк (дальше клавиши идут ему), выделение и текущую ячейку меняет
        ** привязка списка (щелчок по ячейке делает её столбец текущим).
        **
        ** As a mouse click: the focus goes to the row list (the keys go to it next), the list binding changes
        ** the selection and the current cell (a click on a cell makes its column current).
        */
        void pressRow(RouterState& state, ui::Node& row, ui::Node& hit, std::uint8_t modifiers)
        {
            ui::Node& list = *row.parent;

            changeFocus(state, &list);
            state.actions->listPress(list, hit, modifiers);
        }

        // Выбор, добавление и снятие строки (ячейка — через свою строку); false — выделение не тронуто / Selecting, adding and dropping a row (a cell — through its row); false — the selection is untouched
        bool selectRequest(RouterState& state, ui::Node& node, a11y::Request kind)
        {
            ui::Node* row = &node;
            bool selected = false;

            if (node.kind == ui::ElementKind::GRID_CELL)
                row = node.parent;
            if (row == nullptr || row->parent == nullptr || state.actions == nullptr)
                return false;
            selected = a11y::isSelected(*row);
            if ((kind == a11y::Request::ADD_SELECT && !selected) || (kind == a11y::Request::REMOVE_SELECT && selected))
            {
                pressRow(state, *row, node, platform::MODIFIER_CONTROL);
                return true;
            }
            if (kind != a11y::Request::SELECT && kind != a11y::Request::FOCUS)
                return false;
            pressRow(state, *row, node, 0);
            return true;
        }

        // Раскрыть или свернуть ветку той же кнопкой раскрытия, что и мышь; false — листа или уже в нужном состоянии / Expanding or collapsing the branch by the same expander button as the mouse; false — a leaf or already in the needed state
        bool toggleBranch(RouterState& state, const ui::Node& node, bool expand)
        {
            ui::Node* expander = const_cast<ui::Node*>(a11y::itemExpander(node));

            if (expander == nullptr || !expander->content.branch || expander->content.checked == expand)
                return false;
            activate(state, *expander);
            return true;
        }
    }

    bool itemRequest(RouterState& state, ui::Node& node, const a11y::AccessRequest& request, bool& changed)
    {
        a11y::Role role = a11y::roleOf(node);

        // Узел REALIZE — сам список строк: строки записи ещё нет / The REALIZE node is the row list itself: the record row does not exist yet
        if (request.kind == a11y::Request::REALIZE)
        {
            changed = state.actions != nullptr && state.actions->listReveal(node, request.index);
            return true;
        }
        if (role != a11y::Role::DATA_ITEM && role != a11y::Role::TREE_ITEM && role != a11y::Role::CELL)
            return false;
        switch (request.kind)
        {
            case a11y::Request::EXPAND: case a11y::Request::COLLAPSE:
                changed = toggleBranch(state, node, request.kind == a11y::Request::EXPAND);
                return true;
            case a11y::Request::SELECT: case a11y::Request::ADD_SELECT: case a11y::Request::REMOVE_SELECT:
            case a11y::Request::FOCUS:
                changed = selectRequest(state, node, request.kind);
                return true;
            default:
                return false;
        }
    }
}
