#include "gran/a11y/access_tree.hpp"
#include "gran/input/input_router.hpp"

namespace gran::a11y
{
    namespace
    {
        // Поиск узла в видимом поддереве / Searching the node in the visible subtree
        bool containsIn(const ui::Node& current, const ui::Node* node) noexcept
        {
            if (&current == node)
                return true;
            for (const std::unique_ptr<ui::Node>& child : current.children)
            {
                if (child->visible && containsIn(*child, node))
                    return true;
            }
            return false;
        }
    }

    /*
    ** Проверка обходом от корня: клиент UI Automation может держать элемент дольше, чем живёт
    ** узел. Дерево интерфейса — сотни, редко тысячи узлов, обход дешевле учёта ссылок в каждом узле.
    **
    ** A check by walking from the root: a UI Automation client may hold an element longer than
    ** the node lives. A UI tree has hundreds, rarely thousands of nodes; a walk is cheaper than reference tracking in every node.
    */
    bool contains(const AccessTree& tree, const ui::Node* node) noexcept
    {
        if (tree.root == nullptr || node == nullptr)
            return false;
        return containsIn(*tree.root, node);
    }

    // То же попадание, что у мыши (преобразования, отсечение) / The same hit as the mouse has (transforms, clipping)
    const ui::Node* nodeAt(const AccessTree& tree, Vec2 point)
    {
        const ui::Node* hit = nullptr;

        if (tree.root == nullptr)
            return nullptr;
        hit = input::hitTest(*tree.root, point);
        if (hit == nullptr)
            return tree.root;
        return hit;
    }
}
