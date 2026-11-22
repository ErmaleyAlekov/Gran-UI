#include "gran/binding/internal/expression_binding.hpp"

namespace gran::binding
{
    // Переиспользуем порядок привязок, пока состав не изменился.
    // Reuse the binding order until a structural change invalidates it.
    std::shared_ptr<const ExpressionBindings::BindingList> ExpressionBindings::snapshot()
    {
        if (!_snapshot)
            _snapshot = std::make_shared<const BindingList>(_bindings);
        return _snapshot;
    }

    // Снимок удерживает адреса источников отложенных записей до применения всей группы.
    // The snapshot keeps deferred write bindings alive until the whole batch has been applied.
    void ExpressionBindings::writeChanges()
    {
        std::vector<PendingWrite> writes;

        const auto editors = snapshot();
        for (const auto& binding : *editors)
        {
            if (binding->node && !binding->stale && binding->source != SourceKind::NONE)
                collectWrite(*binding, writes);
        }
        for (const PendingWrite& write : writes)
            applyWrite(write);
    }
}
