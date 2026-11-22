#include "gran/layout/internal/layout_internal.hpp"

#include <cmath>

namespace gran::layout::internal
{
    // Для ограничения свободного места нужны исходные коэффициенты.
    // The free-space cap uses the unscaled factors.
    float flexFactor(const FlexItem& item, bool growing) noexcept
    {
        if (growing)
            return item.node->computed.grow;
        return item.node->computed.shrink;
    }

    namespace
    {
        // Доли сжатия пропорциональны базовым размерам.
        // Shrink weights are proportional to each item's base size.
        float weight(const FlexItem& item, bool growing) noexcept
        {
            if (growing)
                return item.node->computed.grow;
            return item.node->computed.shrink * item.base;
        }
    }

    // После заморозки ограничение дробной суммы считается от начального свободного места.
    // After a freeze the fractional-factor cap still uses the initial free space.
    void distributeFree(std::vector<FlexItem>& items, std::size_t first, std::size_t last, const FlexSpace& space)
    {
        float used = 0.0f;
        float weights = 0.0f;
        float factors = 0.0f;

        for (std::size_t index = first; index < last; ++index)
        {
            const FlexItem& item = items[index];
            if (item.frozen)
                used += item.target;
            else
            {
                used += item.base;
                factors += flexFactor(item, space.growing);
                weights += weight(item, space.growing);
            }
        }
        float free = space.available - used;
        if (factors < 1.0f && std::fabs(space.initial * factors) < std::fabs(free))
            free = space.initial * factors;
        for (std::size_t index = first; index < last; ++index)
        {
            FlexItem& item = items[index];
            if (!item.frozen && weights > 0.0f)
                item.target = item.base + free * (weight(item, space.growing) / weights);
        }
    }
}
