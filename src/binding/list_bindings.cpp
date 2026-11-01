#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::binding
{
    ListBindings::ListBindings(RemovalListener* listener)
        : _listener(listener), _expressions(std::make_unique<ExpressionBindings>(*this))
    {
    }

    // Привязки и зеркало отписываются первыми, представления уходят от последнего к первому / The bindings and the mirror unsubscribe first, the views go from the last to the first
    ListBindings::~ListBindings()
    {
        _bindings.clear();
        _expressions.reset();
        _selections.clear();
        while (!_views.empty())
            _views.pop_back();
    }

    /*
    ** Сначала собственные дети, потом сам узел: привязка сразу вставляет копии записей
    ** и привязывает их вложенные списки сама — обход копий второй раз дал бы двойные привязки.
    ** Выделение должно относиться к тому же списку, что и items: иначе номера записей разошлись бы.
    **
    ** The own children first, then the node itself: the binding inserts the record copies at once
    ** and binds their nested lists itself — walking the copies a second time would give double bindings.
    ** The selection must belong to the same list as items: otherwise the record indices would diverge.
    */
    void ListBindings::bind(ui::Node& subtree)
    {
        std::vector<ui::Node*> children;
        data::SelectionModel* chosen = nullptr;

        for (const std::unique_ptr<ui::Node>& child : subtree.children)
            children.push_back(child.get());
        for (ui::Node* child : children)
            bind(*child);
        _expressions->bindNode(subtree);
        if (subtree.content.items.empty() || !subtree.itemTemplate)
            return;
        data::ListSource* source = list(subtree.content.items);
        if (source == nullptr)
            source = &model(subtree.content.items);
        if (!subtree.content.selection.empty())
            chosen = selection(subtree.content.selection);
        if (!subtree.content.selection.empty() && (chosen == nullptr || &chosen->source() != source))
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_SELECTION_MISMATCH,
                {subtree.content.selection, subtree.content.items}));
        _bindings.push_back(std::make_unique<ListBinding>(*this, subtree, *source, chosen));
    }

    // Уничтожение привязки отписывает её от модели / Destroying a binding unsubscribes it from the model
    void ListBindings::clear() noexcept
    {
        _expressions->clear();
        _bindings.clear();
    }
}
