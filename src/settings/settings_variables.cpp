#include "gran/settings/settings.hpp"

namespace gran::settings
{
    // Сохранённая переменная выражений / A saved expression variable
    bool Settings::variable(const std::string& name, Value& value) const
    {
        auto found = _model.variables.find(name);

        if (found == _model.variables.end())
            return false;
        value = found->second;
        return true;
    }

    // Изменение — только если тип или текст другие / A change — only if the type or the text differ
    void Settings::setVariable(const std::string& name, const Value& value)
    {
        auto found = _model.variables.find(name);

        if (found != _model.variables.end() && found->second.type == value.type && found->second.text == value.text)
            return;
        _model.variables[name] = value;
        _modified = true;
    }

    // Путь файла настроек / The settings file path
    const std::string& Settings::path() const noexcept
    {
        return _path;
    }

    // Есть ли несохранённые изменения / Whether there are unsaved changes
    bool Settings::modified() const noexcept
    {
        return _modified;
    }
}
