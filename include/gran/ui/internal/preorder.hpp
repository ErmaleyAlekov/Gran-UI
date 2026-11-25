#pragma once

#include <vector>

namespace gran::ui::internal
{
    // Обход поддерева в порядке разметки / Walk a subtree in markup order
    template<class NodeType>
    class Preorder
    {
    public:
        explicit Preorder(NodeType& root, bool includeTemplates = false)
            : _pending{&root}, _includeTemplates(includeTemplates) {}

        NodeType* next()
        {
            if (_pending.empty()) return nullptr;
            NodeType* node = _pending.back();
            _pending.pop_back();
            if (_includeTemplates && node->itemTemplate)
                _pending.push_back(node->itemTemplate.get());
            for (auto it = node->children.rbegin(); it != node->children.rend(); ++it)
                _pending.push_back(it->get());
            return node;
        }

    private:
        std::vector<NodeType*> _pending;
        bool _includeTemplates;
    };
}
