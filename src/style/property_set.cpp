#include "gran/style/property.hpp"

#include <utility>

namespace gran::style
{
    // Повторное объявление заменяет прежнее на том же месте
    // A repeated declaration replaces the former one in place
    void setDeclaration(PropertySet& set, PropertyId id, PropertyValue value)
    {
        for (Declaration& declaration : set)
        {
            if (declaration.first == id)
            {
                declaration.second = std::move(value);
                return;
            }
        }
        set.emplace_back(id, std::move(value));
    }

    // Линейный поиск: наборы малы (единицы объявлений)
    // A linear lookup: the sets are small (a few declarations)
    const PropertyValue* findDeclaration(const PropertySet& set, PropertyId id) noexcept
    {
        for (const Declaration& declaration : set)
        {
            if (declaration.first == id)
                return &declaration.second;
        }
        return nullptr;
    }
}
