#include "gran/a11y/access_tree.hpp"
#include "gran/style/style_sheet.hpp"
#include "gran/ui/tree_view.hpp"

namespace gran::a11y
{
    /*
    ** Строка списка выделена моделью выделения (привязка ставит ей состояние стиля Selected), у радиокнопки
    ** и вкладки выбор — отметка checked.
    **
    ** A list row is selected by the selection model (the binding gives it the Selected style state), for a radio button
    ** and a tab the choice is the checked mark.
    */
    bool isSelected(const ui::Node& node) noexcept
    {
        if (node.kind == ui::ElementKind::GRID_ROW || node.kind == ui::ElementKind::TREE_ROW)
            return (node.stateMask & style::stateBit(style::State::SELECTED)) != 0;
        return node.content.checked;
    }

    // Строка ищет кнопку среди детей и в первой позиции ячеек, ячейка — у себя первой / A row looks for the button among the children and first in the cells, a cell — first in itself
    const ui::Node* itemExpander(const ui::Node& node) noexcept
    {
        if (node.kind == ui::ElementKind::GRID_ROW || node.kind == ui::ElementKind::TREE_ROW)
            return ui::rowExpander(const_cast<ui::Node&>(node));
        if (node.kind != ui::ElementKind::GRID_CELL || node.children.empty())
            return nullptr;
        if (node.children.front()->kind != ui::ElementKind::TREE_EXPANDER)
            return nullptr;
        return node.children.front().get();
    }

    /*
    ** Пункт меню раскрыт, когда его подменю показано; строка дерева — по кнопке раскрытия: признак ветки
    ** и отметка раскрытия (их ставит привязка списка). Строка без детей — «лист», а не «свёрнута»: так диктор
    ** не обещает ветку, которой нет.
    **
    ** A menu item is expanded when its submenu is shown; a tree row — by the expander button: the branch flag
    ** and the expanded mark (the list binding sets them). A row without children is a "leaf", not "collapsed": so the screen
    ** reader does not promise a branch that is not there.
    */
    ExpandState expandStateOf(const ui::Node& node) noexcept
    {
        const ui::Node* expander = itemExpander(node);

        if (expander == nullptr)
        {
            const ui::Node* submenu = nullptr;
            if (node.kind == ui::ElementKind::MENU_ITEM)
                submenu = ui::submenuOf(node);
            if (submenu == nullptr)
                return ExpandState::NONE;
            if (submenu->visible)
                return ExpandState::EXPANDED;
            return ExpandState::COLLAPSED;
        }
        if (!expander->content.branch)
            return ExpandState::LEAF;
        if (expander->content.checked)
            return ExpandState::EXPANDED;
        return ExpandState::COLLAPSED;
    }

    // Стрелка — второй ребёнок заголовка столбца (первый — название), её текст «▲»/«▼» диктор прочёл бы как символ
    // The arrow is the second child of a column header (the first is the title), a screen reader would read its "▲"/"▼" text as a symbol
    bool hiddenText(const ui::Node& node) noexcept
    {
        const ui::Node* parent = node.parent;

        if (node.kind != ui::ElementKind::TEXT)
            return false;
        if (node.content.text.empty() && node.children.empty())
            return true;
        return parent != nullptr && parent->kind == ui::ElementKind::GRID_HEADER_CELL && !parent->children.empty()
            && parent->children.front().get() != &node;
    }
}
