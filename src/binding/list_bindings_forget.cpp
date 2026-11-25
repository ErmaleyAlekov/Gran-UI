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

    /*
    ** Уничтожение привязки отписывает её от модели. Таблица со строкой фильтров уходит (ветка удалена, документ
    ** перезагружен, окно закрыто) — её условия снимаются с модели: иначе прокси или дерево остались бы отфильтрованы
    ** без видимой причины (ревью Т-72). Снимаются после отписки: строки уходящей таблицы не перестраиваются зря.
    **
    ** Destroying a binding unsubscribes it from the model. A table with a filter row goes away (a branch is removed, the document
    ** is reloaded, the window is closed) — its conditions are dropped from the model: otherwise the proxy or the tree would stay
    ** filtered for no visible reason (review T-72). They are dropped after unsubscribing: the leaving table rows are not rebuilt in vain.
    */
    void ListBindings::forget(const ui::Node& subtree)
    {
        auto filtered = documentFilters(subtree);
        discardDocumentBindings(subtree);
        for (auto* model : filtered) applyColumnConditions(*model, {});
    }

    std::vector<data::ListSource*> ListBindings::documentFilters(const ui::Node& subtree) const
    {
        std::vector<data::ListSource*> filtered;
        for (const auto& binding : _bindings)
        {
            if (contains(subtree, &binding->container()) && ownsColumnFilter(binding->container()))
                filtered.push_back(&binding->model());
        }
        return filtered;
    }

    void ListBindings::releaseDocumentFilters(const ui::Node& subtree)
    {
        for (auto* model : documentFilters(subtree)) applyColumnConditions(*model, {});
    }

    void ListBindings::discardDocumentBindings(const ui::Node& subtree)
    {
        _expressions->forget(subtree);
        for (std::size_t index = 0; index < _bindings.size();)
        {
            if (!contains(subtree, &_bindings[index]->container()))
            {
                ++index;
                continue;
            }
            _bindings.erase(_bindings.begin() + static_cast<std::ptrdiff_t>(index));
        }
    }

}
