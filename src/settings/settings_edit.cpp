#include "gran/settings/settings.hpp"

#include <cstdio>

namespace gran::settings
{
    // Строка заменяет прежнее значение любого типа / A string replaces the former value of any type
    void Settings::setString(const std::string& name, const std::string& value)
    {
        _model.values[name] = Value{ValueType::STRING, value, {}};
        _modified = true;
    }

    // 17 значащих цифр — число читается обратно без потерь; «.» не зависит от локали (snprintf в «C») / 17 significant digits — the number reads back losslessly
    void Settings::setNumber(const std::string& name, double value)
    {
        char text[40];

        std::snprintf(text, sizeof(text), "%.17g", value);
        _model.values[name] = Value{ValueType::NUMBER, text, {}};
        _modified = true;
    }

    // Логическое — словами xs:boolean / A boolean — in the xs:boolean words
    void Settings::setBool(const std::string& name, bool value)
    {
        std::string text = "false";

        if (value)
            text = "true";
        _model.values[name] = Value{ValueType::BOOLEAN, text, {}};
        _modified = true;
    }

    // Список строк (последние файлы и т. п.) / A list of strings (the recent files and so on)
    void Settings::setList(const std::string& name, const std::vector<std::string>& items)
    {
        _model.values[name] = Value{ValueType::LIST, std::string(), items};
        _modified = true;
    }

    // true — значение было / true — the value existed
    bool Settings::remove(const std::string& name)
    {
        if (_model.values.erase(name) == 0)
            return false;
        _modified = true;
        return true;
    }
}
