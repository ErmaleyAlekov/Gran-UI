#include "gran/ui/node.hpp"

namespace gran::ui
{
    namespace
    {
        // Рекурсивная копия полей разметки / A recursive copy of the markup fields
        std::unique_ptr<Node> copyTree(const Node& node)
        {
            std::unique_ptr<Node> copy = std::make_unique<Node>();

            copy->kind = node.kind;
            copy->id = node.id;
            copy->rule = node.rule;
            copy->declared = node.declared;
            copy->content = node.content;
            copy->visible = node.visible;
            copy->enabled = node.enabled;
            copy->line = node.line;
            copy->column = node.column;
            for (const std::unique_ptr<Node>& child : node.children)
                appendChild(*copy, copyTree(*child));
            return copy;
        }
    }

    // Поиск в глубину: сначала сам узел, затем дети по порядку / A depth-first search: the node itself first, then the children in order
    Node* findNode(Node& subtree, const std::string& id) noexcept
    {
        if (subtree.id == id)
            return &subtree;
        for (const std::unique_ptr<Node>& child : subtree.children)
        {
            Node* found = findNode(*child, id);
            if (found != nullptr)
                return found;
        }
        return nullptr;
    }

    /*
    ** Новый узел стоит «грязным» целиком (значения полей по умолчанию), поэтому проход
    ** стилей и компоновка обработают копию, как только она окажется в дереве; начальные
    ** состояния (DISABLED, CHECKED) выставляются заново из enabled и checked.
    **
    ** A new node is entirely "dirty" (the default field values), so the style pass
    ** and the layout process the copy as soon as it gets into the tree; the initial
    ** states (DISABLED, CHECKED) are set anew from enabled and checked.
    */
    std::unique_ptr<Node> cloneTree(const Node& node)
    {
        std::unique_ptr<Node> copy = copyTree(node);

        applyInitialStates(*copy);
        return copy;
    }

    // Участки — только у Text, и все его дети — SPAN (их создаёт загрузчик разметки или код)
    // Runs exist only for a Text, and all its children are SPAN (made by the markup loader or by code)
    bool hasSpans(const Node& node) noexcept
    {
        return node.kind == ElementKind::TEXT && !node.children.empty()
            && node.children.front()->kind == ElementKind::SPAN;
    }

    // Склейка текстов детей; меняется ширина текста — нужна компоновка / Joining the children texts; the text width changes — layout is needed
    void joinSpans(Node& node)
    {
        std::string joined;

        for (const std::unique_ptr<Node>& child : node.children)
            joined += child->content.text;
        node.content.text = joined;
        markDirty(node, DIRTY_LAYOUT | DIRTY_PAINT);
    }
}
