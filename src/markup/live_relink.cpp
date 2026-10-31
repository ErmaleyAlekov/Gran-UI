#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <unordered_map>

namespace gran::markup::internal
{
    namespace
    {
        using ElementMap = std::unordered_map<const beresta::Element*, const beresta::Element*>;

        // Равные деревья обходятся параллельно: i-й ребёнок-элемент одного соответствует i-му другого
        // Equal trees are walked in parallel: the i-th element child of one matches the i-th of the other
        void mapElements(const beresta::Element& previous, const beresta::Element& next, ElementMap& map)
        {
            const beresta::Element* left = firstElementChild(previous);
            const beresta::Element* right = firstElementChild(next);

            map[&previous] = &next;
            while (left != nullptr && right != nullptr)
            {
                mapElements(*left, *right, map);
                left = nextElementSibling(*left);
                right = nextElementSibling(*right);
            }
        }

        // Узлы поддерева (и шаблоны списков) получают пары «узел → новый элемент»; узлы без элемента пропускаются
        // The subtree nodes (and the list templates) get "node → new element" pairs; the nodes without an element are skipped
        void relinkTree(ui::Node& node, const ElementMap& map, PatchPlan& plan)
        {
            ElementMap::const_iterator found = map.find(node.domElement);

            if (node.domElement != nullptr && found != map.end())
                plan.relink.emplace_back(&node, found->second);
            if (node.itemTemplate)
                relinkTree(*node.itemTemplate, map, plan);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                relinkTree(*child, map, plan);
        }
    }

    // Поддерево не меняется вовсе: копии записей, состояние и слушатели остаются как были
    // The subtree does not change at all: the record copies, the state and the listeners stay as they were
    void relinkEqual(ui::Node& node, const beresta::Element& previous, const beresta::Element& next, PatchPlan& plan)
    {
        ElementMap map;

        mapElements(previous, next, map);
        relinkTree(node, map, plan);
    }
}
