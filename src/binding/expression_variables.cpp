#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    namespace
    {
        // Текст значения по типу: логическое — "true"/"false", число — по правилам XPath ("2", не "2.0") / The value text by type: a boolean is "true"/"false", a number is by the XPath rules ("2", not "2.0")
        std::string normalized(const data::Variable& variable)
        {
            if (variable.type == data::VariableType::BOOLEAN && (variable.value == "true" || variable.value == "1"))
                return "true";
            if (variable.type == data::VariableType::BOOLEAN)
                return "false";
            if (variable.type == data::VariableType::NUMBER)
                return beresta::xpath::numberToString(beresta::xpath::stringToNumber(variable.value));
            return variable.value;
        }
    }

    // Новая версия — только при настоящем изменении: присваивание того же значения ничего не пересчитывает / A new version only on a real change: assigning the same value recomputes nothing
    void ExpressionBindings::setVariable(const data::Variable& variable)
    {
        std::string value = normalized(variable);
        VariableSlot& slot = _variables[variable.name];

        if (slot.version != 0 && slot.value.type == variable.type && slot.value.value == value)
            return;
        slot.value.name = variable.name;
        slot.value.type = variable.type;
        slot.value.value = std::move(value);
        slot.version = ++_clock;
    }

    void ExpressionBindings::setVariableText(const std::string& name, const std::string& text)
    {
        const data::Variable* existing = variable(name);
        data::Variable changed{name, data::VariableType::STRING, text};

        if (existing != nullptr)
            changed.type = existing->type;
        setVariable(changed);
    }

    const data::Variable* ExpressionBindings::variable(std::string_view name) const noexcept
    {
        auto found = _variables.find(name);

        if (found == _variables.end())
            return nullptr;
        return &found->second.value;
    }

    std::uint64_t ExpressionBindings::variableVersion(std::string_view name) const noexcept
    {
        auto found = _variables.find(name);

        if (found == _variables.end())
            return 0;
        return found->second.version;
    }
}
