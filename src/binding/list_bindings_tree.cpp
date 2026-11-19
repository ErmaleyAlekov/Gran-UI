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
        std::vector<data::ListSource*> filtered;

        _expressions->forget(subtree);
        for (std::size_t index = 0; index < _bindings.size();)
        {
            if (!contains(subtree, &_bindings[index]->container()))
            {
                ++index;
                continue;
            }
            if (ownsColumnFilter(_bindings[index]->container()))
                filtered.push_back(&_bindings[index]->model());
            _bindings.erase(_bindings.begin() + static_cast<std::ptrdiff_t>(index));
        }
        for (data::ListSource* model : filtered)
            applyColumnConditions(*model, {});
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

    // Заголовки — дети полосы, первого ребёнка Tabs; syncTabs сохраняет выбранное значение / The headers are the strip children, the first Tabs child; syncTabs keeps the selected value
    void ListBindings::resyncTabs(ui::Node& tabs)
    {
        if (tabs.kind != ui::ElementKind::TABS)
            return;
        if (_listener != nullptr && !tabs.children.empty() && tabs.children.front()->kind == ui::ElementKind::TAB_BAR)
        {
            for (const std::unique_ptr<ui::Node>& header : tabs.children.front()->children)
                _listener->nodeRemoving(*header);
        }
        ui::syncTabs(tabs);
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
