#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::binding
{
    namespace
    {
        // Item узла с вложенными Item его детей / The node Item with the nested Items of its children
        void appendNodes(const data::TreeModel& tree, data::NodeId parent, beresta::Element& target)
        {
            for (std::size_t row = 0; row < tree.childCount(parent); ++row)
            {
                data::NodeId child = tree.child(parent, row);
                beresta::Node* item = target.appendChild(mirrorItem(tree.record(child)));
                appendNodes(tree, child, static_cast<beresta::Element&>(*item));
            }
        }
    }

    TreeMirror::TreeMirror(std::string modelName, data::TreeModel& model, beresta::Element& target)
        : MirrorSlot(std::move(modelName), target), tree(model)
    {
        appendNodes(tree, tree.root(), element);
        tree.subscribe(this);
    }

    TreeMirror::~TreeMirror()
    {
        tree.unsubscribe(this);
    }

    void TreeMirror::rebuild()
    {
        if (!dirty)
            return;
        dirty = false;
        element.clearChildren();
        appendNodes(tree, tree.root(), element);
    }
}
