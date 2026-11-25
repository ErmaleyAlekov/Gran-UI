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
            struct Frame { data::NodeId node; beresta::Element* target; std::size_t row; };
            std::vector<Frame> stack{{parent, &target, 0}};
            while (!stack.empty())
            {
                Frame& frame = stack.back();
                if (frame.row == tree.childCount(frame.node))
                {
                    stack.pop_back();
                    continue;
                }
                const auto child = tree.child(frame.node, frame.row++);
                auto* item = frame.target->appendChild(mirrorItem(tree.record(child)));
                stack.push_back({child, static_cast<beresta::Element*>(item), 0});
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
        beresta::Element replacement(name);
        appendNodes(tree, tree.root(), replacement);
        element = std::move(replacement);
        dirty = false;
    }
}
