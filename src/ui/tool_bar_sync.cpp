#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        /*
        ** Зеркала идут в том же порядке, что и дети панели (группы развёрнуты): next — номер следующего зеркала.
        ** Зеркало видно, когда его узел панели ушёл в меню (shown) и сам не скрыт приложением.
        **
        ** The mirrors go in the same order as the toolbar children (groups unfolded): next is the number of the next mirror.
        ** A mirror is shown when its toolbar node went into the menu (shown) and is not hidden by the application itself.
        */
        void syncMirror(const Node& source, Node& menu, std::size_t& next, bool shown)
        {
            if (source.kind == ElementKind::TOOL_GROUP)
            {
                for (const std::unique_ptr<Node>& child : source.children)
                    syncMirror(*child, menu, next, shown && child->visible);
                return;
            }
            if (next >= menu.children.size())
                return;
            Node& mirror = *menu.children[next++];
            if (mirror.visible == shown)
                return;
            mirror.visible = shown;
            markDirty(mirror, DIRTY_STYLE | DIRTY_LAYOUT | DIRTY_PAINT);
        }
    }

    // Зеркало видно у ушедших в меню детей; у прочих — скрыто / The mirror is shown for the children that went into the menu; hidden for the others
    void syncOverflowMenu(Node& bar)
    {
        Node* more = toolBarMore(bar);
        std::size_t next = 0;

        if (more == nullptr || more->children.empty())
            return;
        Node& menu = *more->children.front();
        for (const std::unique_ptr<Node>& child : bar.children)
        {
            if (child.get() != more)
                syncMirror(*child, menu, next, child->content.overflowed);
        }
    }

    /*
    ** Полная длина панели нужна родителю (горизонтальный ряд, рост окна): ушедшие дети возвращаются на время
    ** замера; размещение (fitToolBar) снова решит, кому уйти.
    **
    ** The parent needs the full toolbar length (a horizontal row, a growing window): the children that went return for the
    ** measuring; the placement (fitToolBar) decides again who goes.
    */
    void unfoldToolBar(Node& bar) noexcept
    {
        Node* more = toolBarMore(bar);

        for (const std::unique_ptr<Node>& child : bar.children)
        {
            if (!child->content.overflowed)
                continue;
            child->content.overflowed = false;
            child->visible = true;
        }
        if (more != nullptr)
            more->visible = false;
    }
}
