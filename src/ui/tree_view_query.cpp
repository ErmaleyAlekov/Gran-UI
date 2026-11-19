#include "gran/ui/tree_view.hpp"

namespace gran::ui
{
    Node* treeViewRows(Node& view) noexcept
    {
        if (view.kind != ElementKind::TREE_VIEW || view.children.empty())
            return nullptr;
        Node& scroll = *view.children.front();
        if (scroll.kind != ElementKind::SCROLL || scroll.children.empty())
            return nullptr;
        return scroll.children.front().get();
    }

    /*
    ** Привязка пишет в value кнопки текст логического выражения $hasChildren: «true» или «false».
    **
    ** The binding writes the text of the $hasChildren boolean expression into the button value: "true" or "false".
    */
    bool expanderHasBranch(const Node& expander) noexcept
    {
        return expander.kind == ElementKind::TREE_EXPANDER && expander.content.option == "true";
    }
}
