#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    const beresta::Element* DataMirror::record(const data::ListModel& model, std::size_t index) const
    {
        for (const std::unique_ptr<MirrorEntry>& entry : _entries)
        {
            if (&entry->model != &model)
                continue;
            if (index >= entry->element.getChildCount())
                return nullptr;
            return static_cast<const beresta::Element*>(entry->element.getChild(index));
        }
        return nullptr;
    }

    // Моделей единицы — линейный поиск дешевле словаря / The models are few — a linear search is cheaper than a map
    std::uint64_t DataMirror::version(std::string_view name) const noexcept
    {
        for (const std::unique_ptr<MirrorEntry>& entry : _entries)
        {
            if (entry->name == name)
                return entry->version;
        }
        return 0;
    }

    std::uint64_t DataMirror::version(const data::ListModel& model) const noexcept
    {
        for (const std::unique_ptr<MirrorEntry>& entry : _entries)
        {
            if (&entry->model == &model)
                return entry->version;
        }
        return 0;
    }
}
