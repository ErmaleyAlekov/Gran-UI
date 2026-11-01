#include "gran/app/internal/app_state.hpp"

#include <locale>
#include <sstream>
#include <utility>

namespace gran
{
    void Application::setVariable(const std::string& name, const std::string& value)
    {
        _state->lists.setVariableText(name, value);
    }

    /*
    ** Классическая локаль: десятичная точка всегда «.», как требует XPath (std::to_chars для double нет
    ** в GCC 8 сборки Astra 1.7). 17 знаков — без потери точности; запись XPath ("0.1") даст нормализация.
    **
    ** The classic locale: the decimal point is always ".", as XPath requires (std::to_chars for double is missing
    ** in GCC 8 of the Astra 1.7 build). 17 digits — without precision loss; the normalization gives the XPath form ("0.1").
    */
    void Application::setVariableNumber(const std::string& name, double value)
    {
        std::ostringstream text;

        text.imbue(std::locale::classic());
        text.precision(17);
        text << value;
        _state->lists.setVariable(data::Variable{name, data::VariableType::NUMBER, text.str()});
    }

    void Application::setVariableBoolean(const std::string& name, bool value)
    {
        data::Variable variable{name, data::VariableType::BOOLEAN, "false"};

        if (value)
            variable.value = "true";
        _state->lists.setVariable(variable);
    }

    std::optional<std::string> Application::variable(const std::string& name) const
    {
        const data::Variable* found = _state->lists.variable(name);

        if (found == nullptr)
            return std::nullopt;
        return found->value;
    }

    void Application::setConverter(const std::string& name, binding::Converter converter)
    {
        _state->lists.setConverter(name, std::move(converter));
    }
}
