#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    ListBindings::ListBindings(RemovalListener* listener)
        : _listener(listener), _expressions(std::make_unique<ExpressionBindings>(*this))
    {
    }

    // Привязки уничтожаются раньше моделей (порядок полей) / The bindings are destroyed before the models (the field order)
    ListBindings::~ListBindings() = default;

    // Модель живёт в куче: ссылки на неё стабильны при росте словаря / The model lives on the heap: references to it are stable when the map grows
    data::ListModel& ListBindings::model(const std::string& name)
    {
        std::unique_ptr<data::ListModel>& slot = _models[name];

        if (!slot)
        {
            slot = std::make_unique<data::ListModel>();
            _expressions->watchModel(name, *slot);
        }
        return *slot;
    }

    /*
    ** Сначала собственные дети, потом сам узел: привязка сразу вставляет копии записей
    ** и привязывает их вложенные списки сама — обход копий второй раз дал бы двойные привязки.
    **
    ** The own children first, then the node itself: the binding inserts the record copies at once
    ** and binds their nested lists itself — walking the copies a second time would give double bindings.
    */
    void ListBindings::bind(ui::Node& subtree)
    {
        std::vector<ui::Node*> children;

        for (const std::unique_ptr<ui::Node>& child : subtree.children)
            children.push_back(child.get());
        for (ui::Node* child : children)
            bind(*child);
        _expressions->bindNode(subtree);
        if (subtree.content.items.empty() || !subtree.itemTemplate)
            return;
        data::ListModel& source = model(subtree.content.items);
        _bindings.push_back(std::make_unique<ListBinding>(*this, subtree, source));
    }

    // Уничтожение привязки отписывает её от модели / Destroying a binding unsubscribes it from the model
    void ListBindings::clear() noexcept
    {
        _expressions->clear();
        _bindings.clear();
    }
}
