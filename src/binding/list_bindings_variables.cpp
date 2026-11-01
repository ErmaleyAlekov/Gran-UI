#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"

#include <utility>

namespace gran::binding
{
    void ListBindings::setVariable(const data::Variable& variable)
    {
        _expressions->setVariable(variable);
    }

    void ListBindings::setVariableText(const std::string& name, const std::string& text)
    {
        _expressions->setVariableText(name, text);
    }

    const data::Variable* ListBindings::variable(std::string_view name) const noexcept
    {
        return _expressions->variable(name);
    }

    void ListBindings::setConverter(const std::string& name, Converter converter)
    {
        _expressions->setConverter(name, std::move(converter));
    }

    // Перезагрузка разметки возвращает объявленные значения, как <Data> возвращает записи / A markup reload restores the declared values, as <Data> restores the records
    void ListBindings::adoptVariables(std::vector<data::Variable>& variables)
    {
        for (const data::Variable& variable : variables)
            _expressions->setVariable(variable);
        variables.clear();
    }
}
