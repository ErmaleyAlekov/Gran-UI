#ifndef GRAN_BINDING_INTERNAL_RELOAD_CHECKPOINT_HPP
#define GRAN_BINDING_INTERNAL_RELOAD_CHECKPOINT_HPP

#include "gran/binding/list_bindings.hpp"
#include "gran/data/internal/observer_errors.hpp"
#include <set>

namespace gran::binding
{
    // Сохраняем содержимое моделей, сохраняя адреса объектов и подписки приложения.
    // Save model contents while retaining object addresses and application subscriptions.
    class ReloadCheckpoint
    {
    public:
        ReloadCheckpoint(ListBindings& lists, const markup::UiDocument& next);
        ~ReloadCheckpoint();
        bool published(const std::string& name) const;
        void check() const;
        void restore();
        void rememberSource(data::SortFilterProxy& proxy);
        void rememberSource(data::TreeList& view);
    private:
        void save(data::ListModel& list);
        void save(data::TreeModel& tree);
        void save(data::SelectionModel& selection);
        void save(data::TreeList& view);
        void save(data::SortFilterProxy& proxy);
        void saveRegistry(ListBindings& lists);
        data::internal::ObserverErrors errors;
        ListBindings& owner;
        std::set<std::string> originalNames;
        std::vector<std::function<void()>> undo;
        std::vector<std::function<void()>> undoSources;
    };
}
#endif
