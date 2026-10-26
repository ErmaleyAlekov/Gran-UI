#include "gran/a11y/access_tree.hpp"

namespace gran::a11y
{
    namespace
    {
        // Первый (или последний) видимый ребёнок / The first (or the last) visible child
        const ui::Node* edgeChild(const ui::Node& node, bool last) noexcept
        {
            std::size_t count = node.children.size();

            for (std::size_t step = 0; step < count; ++step)
            {
                std::size_t index = step;
                if (last)
                    index = count - 1 - step;
                if (node.children[index]->visible)
                    return node.children[index].get();
            }
            return nullptr;
        }

        // Ближайший видимый сосед вперёд (+1) или назад (−1) / The nearest visible sibling forward (+1) or backward (−1)
        const ui::Node* sibling(const ui::Node& node, int step) noexcept
        {
            const ui::Node* parent = node.parent;
            std::size_t index = 0;

            if (parent == nullptr)
                return nullptr;
            while (index < parent->children.size() && parent->children[index].get() != &node)
                ++index;
            for (long long next = static_cast<long long>(index) + step;
                 next >= 0 && next < static_cast<long long>(parent->children.size()); next += step)
            {
                if (parent->children[static_cast<std::size_t>(next)]->visible)
                    return parent->children[static_cast<std::size_t>(next)].get();
            }
            return nullptr;
        }
    }

    // Корень дерева доступности — граница: выше и вбок от него ничего нет / The accessibility tree root is a border: nothing above or beside it
    const ui::Node* navigate(const AccessTree& tree, const ui::Node& node, Direction direction) noexcept
    {
        bool root = &node == tree.root;

        switch (direction)
        {
            case Direction::FIRST_CHILD: return edgeChild(node, false);
            case Direction::LAST_CHILD: return edgeChild(node, true);
            default: break;
        }
        if (root)
            return nullptr;
        if (direction == Direction::PARENT)
            return node.parent;
        if (direction == Direction::NEXT)
            return sibling(node, 1);
        return sibling(node, -1);
    }
}
