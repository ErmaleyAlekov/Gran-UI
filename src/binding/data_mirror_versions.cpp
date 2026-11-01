#include "gran/binding/internal/expression_binding.hpp"

#include "beresta.hpp"

namespace gran::binding
{
    const beresta::Element* DataMirror::record(const data::ListSource& model, std::size_t index) const
    {
        for (const std::unique_ptr<MirrorSlot>& entry : _entries)
        {
            if (entry->list() != &model)
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
        for (const std::unique_ptr<MirrorSlot>& entry : _entries)
        {
            if (entry->name == name)
                return entry->version;
        }
        return 0;
    }

    std::uint64_t DataMirror::version(const data::ListSource& model) const noexcept
    {
        for (const std::unique_ptr<MirrorSlot>& entry : _entries)
        {
            if (entry->list() == &model)
                return entry->version;
        }
        return 0;
    }

    const data::ListSource* MirrorSlot::list() const noexcept
    {
        return nullptr;
    }

    const data::ListSource* MirrorEntry::list() const noexcept
    {
        return &model;
    }
}
