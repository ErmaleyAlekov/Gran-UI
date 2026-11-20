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

    /*
    ** Общий предок прежнего контейнера и родителя уходящего узла. Прежний контейнер мог быть удалён в обход
    ** уведомления (поле живёт до конца кадра) — тогда он не в дереве, и контейнером становится сам родитель.
    **
    ** The common ancestor of the former container and the parent of the leaving node. The former container might be removed bypassing
    ** the notification (the field lives until the end of the frame) — then it is not in the tree, and the parent itself becomes the container.
    */
    void noteRemoval(AccessTree& tree, const ui::Node& node) noexcept
    {
        const ui::Node* parent = node.parent;

        if (parent == nullptr || tree.root == nullptr)
            return;
        if (tree.changed == nullptr || !contains(tree, tree.changed))
        {
            tree.changed = parent;
            return;
        }
        for (const ui::Node* candidate = tree.changed; candidate != nullptr; candidate = candidate->parent)
        {
            for (const ui::Node* up = parent; up != nullptr; up = up->parent)
            {
                if (up == candidate)
                {
                    tree.changed = candidate;
                    return;
                }
            }
        }
        tree.changed = tree.root;
    }

    // Нехватка памяти на список — не беда: событие контейнера всё равно уйдёт / Out of memory for the list is no trouble: the container event goes anyway
    void noteAddition(AccessTree& tree, const ui::Node& node) noexcept
    {
        noteRemoval(tree, node);
        if (tree.added.size() >= ADDED_LIMIT)
            return;
        try
        {
            tree.added.push_back(&node);
        }
        catch (...)
        {
            tree.added.clear();
        }
    }
}
