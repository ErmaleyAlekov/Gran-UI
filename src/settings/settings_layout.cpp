#include "gran/settings/settings.hpp"

namespace gran::settings
{
    // Место окна по имени раскладки (<Window layout>) / The window place by the layout name (<Window layout>)
    bool Settings::windowPlacement(const std::string& layout, platform::WindowPlacement& placement) const
    {
        auto found = _model.windows.find(layout);

        if (found == _model.windows.end())
            return false;
        placement = found->second;
        return true;
    }

    // Изменение отмечается, только если место другое: закрытие окна без движения не переписывает файл / A change is marked only if the place differs
    void Settings::setWindowPlacement(const std::string& layout, const platform::WindowPlacement& placement)
    {
        auto found = _model.windows.find(layout);

        if (found != _model.windows.end() && found->second.x == placement.x && found->second.y == placement.y
            && found->second.width == placement.width && found->second.height == placement.height
            && found->second.maximized == placement.maximized)
            return;
        _model.windows[layout] = placement;
        _modified = true;
    }

    // Сохранённое значение атрибута узла / The saved node attribute value
    bool Settings::attribute(const AttributeKey& key, std::string& value) const
    {
        auto found = _model.attributes.find(key);

        if (found == _model.attributes.end())
            return false;
        value = found->second;
        return true;
    }

    // То же правило «только если другое» / The same "only if different" rule
    void Settings::setAttribute(const AttributeKey& key, const std::string& value)
    {
        auto found = _model.attributes.find(key);

        if (found != _model.attributes.end() && found->second == value)
            return;
        _model.attributes[key] = value;
        _modified = true;
    }
}
