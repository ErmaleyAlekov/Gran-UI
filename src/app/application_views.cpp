#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran
{
    namespace
    {
        // Ошибка «нет такой модели» / The "no such model" error
        [[noreturn]] void unknownModel(const std::string& name)
        {
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_UNKNOWN_MODEL, {name}));
        }
    }

    data::SelectionModel& Application::createSelection(const std::string& name, const std::string& source,
        data::SelectionMode mode)
    {
        return _state->lists.createSelection(name, source, mode);
    }

    data::SortFilterProxy& Application::proxy(const std::string& name)
    {
        data::SortFilterProxy* found = _state->lists.proxy(name);

        if (found == nullptr)
            unknownModel(name);
        return *found;
    }

    data::TreeList& Application::treeList(const std::string& name)
    {
        data::TreeList* found = _state->lists.treeList(name);

        if (found == nullptr)
            unknownModel(name);
        return *found;
    }

    data::SelectionModel& Application::selection(const std::string& name)
    {
        data::SelectionModel* found = _state->lists.selection(name);

        if (found == nullptr)
            unknownModel(name);
        return *found;
    }
}
