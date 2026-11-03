#include "gran/settings/settings.hpp"

#include <cstdlib>

namespace gran::settings
{
    // Строку даёт значение любого типа, кроме списка (число и логическое хранятся текстом) / Any value type but a list gives a string (a number and a boolean are kept as text)
    std::string Settings::getString(const std::string& name, const std::string& fallback) const
    {
        auto found = _model.values.find(name);

        if (found == _model.values.end() || found->second.type == ValueType::LIST)
            return fallback;
        return found->second.text;
    }

    // Число разбирается без учёта локали программы («.» — всегда разделитель) / The number is parsed without the program locale ("." is always the separator)
    double Settings::getNumber(const std::string& name, double fallback) const
    {
        auto found = _model.values.find(name);
        char* end = nullptr;

        if (found == _model.values.end() || found->second.type != ValueType::NUMBER)
            return fallback;
        double value = std::strtod(found->second.text.c_str(), &end);
        if (end == found->second.text.c_str())
            return fallback;
        return value;
    }

    // «true» / «false», как xs:boolean / "true" / "false", as xs:boolean
    bool Settings::getBool(const std::string& name, bool fallback) const
    {
        auto found = _model.values.find(name);

        if (found == _model.values.end() || found->second.type != ValueType::BOOLEAN)
            return fallback;
        return found->second.text == "true";
    }

    // Указатель живёт до следующей правки настроек / The pointer lives until the next settings edit
    const Value* Settings::find(const std::string& name) const
    {
        auto found = _model.values.find(name);

        if (found == _model.values.end())
            return nullptr;
        return &found->second;
    }

    // Нет списка — пустой / No list — an empty one
    std::vector<std::string> Settings::getList(const std::string& name) const
    {
        auto found = _model.values.find(name);

        if (found == _model.values.end() || found->second.type != ValueType::LIST)
            return std::vector<std::string>();
        return found->second.items;
    }
}
