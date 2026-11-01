#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::binding
{
    namespace
    {
        // Ошибка «нет такой модели» / The "no such model" error
        [[noreturn]] void unknownModel(const std::string& name)
        {
            throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::DATA_UNKNOWN_MODEL, {name}));
        }
    }

    // Представление отражается для выражений под своим именем, как обычный список / A view is mirrored for expressions under its name, as an ordinary list
    data::SortFilterProxy& ListBindings::createProxy(const std::string& name, const std::string& source)
    {
        data::ListSource* base = list(source);

        if (base == nullptr)
            unknownModel(source);
        checkName(name, "view");
        std::unique_ptr<data::SortFilterProxy> proxy = std::make_unique<data::SortFilterProxy>(*base);
        data::SortFilterProxy& result = *proxy;
        _views.push_back(View{name, std::move(proxy)});
        _expressions->watchModel(name, result);
        return result;
    }

    data::TreeList& ListBindings::createTreeList(const std::string& name, const std::string& tree)
    {
        auto base = _trees.find(tree);

        if (base == _trees.end())
            unknownModel(tree);
        checkName(name, "view");
        std::unique_ptr<data::TreeList> rows = std::make_unique<data::TreeList>(*base->second);
        data::TreeList& result = *rows;
        _views.push_back(View{name, std::move(rows)});
        _expressions->watchModel(name, result);
        return result;
    }

    data::SelectionModel& ListBindings::createSelection(const std::string& name, const std::string& source,
        data::SelectionMode mode)
    {
        data::ListSource* base = list(source);

        if (base == nullptr)
            unknownModel(source);
        checkName(name, "selection");
        std::unique_ptr<data::SelectionModel>& slot = _selections[name];
        slot = std::make_unique<data::SelectionModel>(*base, mode);
        _expressions->watchSelection(name, *slot);
        return *slot;
    }

    // Имена всех видов моделей — одно пространство: выражение tasks/Item не должно гадать, что это / The names of all model kinds are one space: the tasks/Item expression must not guess what it is
    void ListBindings::checkName(const std::string& name, const char* kind) const
    {
        const char* taken = nullptr;

        if (_models.find(name) != _models.end())
            taken = "list";
        else if (_trees.find(name) != _trees.end())
            taken = "tree";
        else if (_selections.find(name) != _selections.end())
            taken = "selection";
        else if (list(name) != nullptr)
            taken = "view";
        if (taken != nullptr)
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::DATA_MODEL_KIND, {name, std::string(taken) + " → " + kind}));
    }
}
