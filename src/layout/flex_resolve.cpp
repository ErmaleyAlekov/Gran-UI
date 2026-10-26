#include "gran/layout/internal/layout_internal.hpp"

#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Коэффициент гибкости: grow при росте, shrink × base при сжатии (CSS «scaled flex shrink factor»)
        // The flex factor: grow when growing, shrink × base when shrinking (the CSS "scaled flex shrink factor")
        float flexFactor(const FlexItem& item, bool growing) noexcept
        {
            if (growing)
                return item.node->computed.grow;
            return item.node->computed.shrink * item.base;
        }

        // Начальная заморозка: нулевой коэффициент или размер уже за пределом
        // The initial freezing: a zero factor or a size already beyond its limit
        void freezeInflexible(std::vector<FlexItem>& items, std::size_t first, std::size_t last, bool growing)
        {
            for (std::size_t index = first; index < last; ++index)
            {
                FlexItem& item = items[index];
                item.target = item.base;
                item.frozen = flexFactor(item, growing) <= 0.0f || (growing && item.base > item.maxMain)
                    || (!growing && item.base < item.minMain);
                if (item.frozen)
                    item.target = clampSize(item.base, item.minMain, item.maxMain);
            }
        }

        // Свободное место делится между незамороженными пропорционально коэффициентам
        // The free space is shared between the unfrozen items in proportion to their factors
        void distributeFree(std::vector<FlexItem>& items, std::size_t first, std::size_t last,
            float available, bool growing)
        {
            float used = 0.0f;
            float factors = 0.0f;

            for (std::size_t index = first; index < last; ++index)
            {
                if (items[index].frozen)
                    used += items[index].target;
                else
                {
                    used += items[index].base;
                    factors += flexFactor(items[index], growing);
                }
            }
            float free = available - used;
            if (growing && factors < 1.0f)
                free *= factors;
            for (std::size_t index = first; index < last; ++index)
            {
                if (!items[index].frozen && factors > 0.0f)
                    items[index].target = items[index].base + free * flexFactor(items[index], growing) / factors;
            }
        }

        /*
        ** Нарушения min/max: при суммарном нарушении > 0 замораживаются упёршиеся в минимум,
        ** при < 0 — в максимум, при 0 — все. Возвращает true, если остались незамороженные.
        **
        ** min/max violations: with a total violation > 0 those hitting the minimum are frozen,
        ** with < 0 — those hitting the maximum, with 0 — all. Returns true if unfrozen items remain.
        */
        bool freezeViolations(std::vector<FlexItem>& items, std::size_t first, std::size_t last)
        {
            float total = 0.0f;
            bool remaining = false;

            for (std::size_t index = first; index < last; ++index)
            {
                if (!items[index].frozen)
                    total += clampSize(items[index].target, items[index].minMain, items[index].maxMain)
                        - items[index].target;
            }
            for (std::size_t index = first; index < last; ++index)
            {
                FlexItem& item = items[index];
                if (item.frozen)
                    continue;
                float clamped = clampSize(item.target, item.minMain, item.maxMain);
                if (total == 0.0f || (total > 0.0f && clamped > item.target) || (total < 0.0f && clamped < item.target))
                    item.frozen = true;
                item.target = clamped;
                remaining = remaining || !item.frozen;
            }
            return remaining;
        }
    }

    // Цикл §9.7: распределить, проверить пределы, заморозить; не больше n + 1 итераций
    // The §9.7 loop: distribute, check the limits, freeze; at most n + 1 iterations
    void resolveFlexibleLengths(std::vector<FlexItem>& items, std::size_t first, std::size_t last, float available)
    {
        float sumBase = 0.0f;

        if (!std::isfinite(available))
            return;
        for (std::size_t index = first; index < last; ++index)
            sumBase += items[index].base;
        bool growing = available >= sumBase;
        freezeInflexible(items, first, last, growing);
        for (std::size_t iteration = first; iteration <= last; ++iteration)
        {
            distributeFree(items, first, last, available, growing);
            if (!freezeViolations(items, first, last))
                break;
        }
    }
}
