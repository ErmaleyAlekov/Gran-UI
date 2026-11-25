#include "gran/binding/internal/expression_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/ui/internal/preorder.hpp"

namespace gran::binding
{
    namespace
    {
        using Kind = markup::DataKind;
        using Kinds = std::map<std::string, Kind>;

        bool listKind(Kind kind)
        {
            return kind == Kind::LIST || kind == Kind::TREE_LIST || kind == Kind::SORT_FILTER;
        }

        // Имена проверяются на копии описаний без уведомлений подписчикам.
        // Check names on a copy of the declarations without notifying observers.
        void declare(Kinds& kinds, const markup::DataSet& set)
        {
            const auto slot = kinds.emplace(set.name, set.kind);
            if (slot.first->second != set.kind)
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::DATA_MODEL_KIND, {set.name, "reload"}));
        }
    }

    void ListBindings::validateDocument(const markup::UiDocument& document) const
    {
        Kinds kinds;
        std::map<std::string, std::string> selections;
        for (const auto& entry : _models) kinds.emplace(entry.first, Kind::LIST);
        for (const auto& entry : _trees) kinds.emplace(entry.first, Kind::TREE);
        for (const auto& view : _views)
            kinds.emplace(view.name, dynamic_cast<data::TreeList*>(view.source.get()) ? Kind::TREE_LIST : Kind::SORT_FILTER);
        for (const auto& entry : _selections) kinds.emplace(entry.first, Kind::SELECTION);
        for (const auto& set : document.data)
            if (set.kind == Kind::LIST || set.kind == Kind::TREE) declare(kinds, set);
        for (const auto& set : document.data)
        {
            if (set.kind == Kind::LIST || set.kind == Kind::TREE) continue;
            if (set.kind != Kind::SELECTION || kinds.find(set.name) == kinds.end())
            {
                const auto source = kinds.find(set.source);
                if (source == kinds.end() || (set.kind == Kind::TREE_LIST ? source->second != Kind::TREE : !listKind(source->second)))
                    throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_UNKNOWN_MODEL, {set.source}));
                if (set.kind == Kind::SELECTION) selections.emplace(set.name, set.source);
            }
            declare(kinds, set);
            if (set.kind != Kind::SELECTION && set.filter.find_first_not_of(" \t\r\n") != std::string::npos)
                ExpressionFilter(*_expressions, set.filter);
        }
        if (!document.root) return;
        ui::internal::Preorder traversal(*document.root, true);
        while (const auto* node = traversal.next())
        {
            const auto& items = node->content.items;
            if (!node->itemTemplate || items.empty()) continue;
            if (!listKind(kinds.emplace(items, Kind::LIST).first->second))
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_MODEL_KIND, {items, "list"}));
            const auto& name = node->content.selection;
            if (name.empty()) continue;
            const auto* chosen = selection(name);
            const auto planned = selections.find(name);
            if (chosen ? &chosen->source() != list(items) : planned == selections.end() || planned->second != items)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_SELECTION_MISMATCH, {name, items}));
        }
    }
}
