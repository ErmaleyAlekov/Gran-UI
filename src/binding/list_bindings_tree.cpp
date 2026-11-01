#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"

#include <utility>

namespace gran::binding
{
    namespace
    {
        // Лежит ли узел в поддереве / Whether the node lies in the subtree
        bool contains(const ui::Node& subtree, const ui::Node* node) noexcept
        {
            for (const ui::Node* current = node; current != nullptr; current = current->parent)
            {
                if (current == &subtree)
                    return true;
            }
            return false;
        }
    }

    // Уничтожение привязки отписывает её от модели / Destroying a binding unsubscribes it from the model
    void ListBindings::forget(const ui::Node& subtree)
    {
        _expressions->forget(subtree);
        for (std::size_t index = 0; index < _bindings.size();)
        {
            if (contains(subtree, &_bindings[index]->container()))
                _bindings.erase(_bindings.begin() + static_cast<std::ptrdiff_t>(index));
            else
                ++index;
        }
    }

    // Сначала снимаются все невладеющие ссылки, затем узел уничтожается / All non-owning references are dropped first, then the node is destroyed
    void ListBindings::removeNode(ui::Node& node)
    {
        ui::Node* parent = node.parent;

        if (parent == nullptr)
            return;
        forget(node);
        if (_listener != nullptr)
            _listener->nodeRemoving(node);
        std::vector<std::unique_ptr<ui::Node>>& siblings = parent->children;
        for (std::size_t index = 0; index < siblings.size(); ++index)
        {
            if (siblings[index].get() == &node)
            {
                siblings.erase(siblings.begin() + static_cast<std::ptrdiff_t>(index));
                break;
            }
        }
        ui::markDirty(*parent, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Подъём от узла: первый предок, чей родитель — привязанный контейнер, и есть копия записи
    // Climbing from the node: the first ancestor whose parent is a bound container is the record copy
    ItemRef ListBindings::itemOf(const ui::Node& node) const noexcept
    {
        for (const ui::Node* current = &node; current->parent != nullptr; current = current->parent)
        {
            for (const std::unique_ptr<ListBinding>& binding : _bindings)
            {
                if (&binding->container() != current->parent)
                    continue;
                std::size_t index = binding->indexOf(*current);
                if (index != static_cast<std::size_t>(-1))
                    return ItemRef{&binding->model(), index, binding->selection()};
            }
        }
        return ItemRef{};
    }
}
