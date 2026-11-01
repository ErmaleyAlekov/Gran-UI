#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    ExpressionBindings::ExpressionBindings(ListBindings& owner)
        : _owner(owner)
    {
    }

    // Привязки уходят раньше зеркала, элементы которого они читали (обратный порядок полей) / The bindings go before the mirror whose elements they read (the reverse field order)
    ExpressionBindings::~ExpressionBindings() = default;

    // Вычисление откладывается до update: копия шаблона ещё не знает своей записи, пока привязка списка строится / The evaluation is put off until update: a template copy does not know its record yet while the list binding is being built
    void ExpressionBindings::bindNode(ui::Node& node)
    {
        for (const ui::AttributeBinding& source : node.content.bindings)
            _bindings.push_back(compileBinding(node, source));
    }

    // Поддерево ещё целое: подъём по родителям от узла привязки безопасен / The subtree is still whole: climbing the parents from the binding node is safe
    void ExpressionBindings::forget(const ui::Node& subtree)
    {
        for (std::size_t index = 0; index < _bindings.size();)
        {
            bool inside = false;
            for (const ui::Node* current = _bindings[index]->node; current != nullptr && !inside; current = current->parent)
                inside = current == &subtree;
            if (inside)
                _bindings.erase(_bindings.begin() + static_cast<std::ptrdiff_t>(index));
            else
                ++index;
        }
    }

    void ExpressionBindings::clear() noexcept
    {
        _bindings.clear();
    }
}
