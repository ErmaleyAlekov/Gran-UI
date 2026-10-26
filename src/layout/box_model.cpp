#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    // Отступ содержимого — сумма padding и ширины рамки / The content inset is padding plus the border width
    Insets contentInsets(const style::ComputedStyle& style) noexcept
    {
        return Insets{style.padding.top + style.borderWidth.top, style.padding.right + style.borderWidth.right,
            style.padding.bottom + style.borderWidth.bottom, style.padding.left + style.borderWidth.left};
    }

    // Проценты считаются от конечной опоры, иначе длина неизвестна (как auto)
    // Percent is taken from a finite reference, otherwise the length is unknown (as auto)
    float resolveLength(const style::Length& length, float reference) noexcept
    {
        if (length.unit == style::LengthUnit::PIXELS)
            return length.value;
        if (length.unit == style::LengthUnit::PERCENT && std::isfinite(reference))
            return reference * length.value / 100.0f;
        return std::numeric_limits<float>::quiet_NaN();
    }

    // Сначала максимум, затем минимум: при min > max побеждает min
    // The maximum first, then the minimum: with min > max the min wins
    float clampSize(float value, float minimum, float maximum) noexcept
    {
        return std::max(minimum, std::min(value, maximum));
    }

    // Бесконечная ось остаётся бесконечной, конечная не уходит в минус
    // An infinite axis stays infinite, a finite one does not go negative
    Size shrinkAvailable(Size available, const Insets& insets) noexcept
    {
        Size result = available;

        if (std::isfinite(result.width))
            result.width = std::max(0.0f, result.width - insets.left - insets.right);
        if (std::isfinite(result.height))
            result.height = std::max(0.0f, result.height - insets.top - insets.bottom);
        return result;
    }

    // Ширина или высота задана не auto / The width or the height is not auto
    bool hasFixedSize(const ui::Node& node, bool horizontal) noexcept
    {
        if (horizontal)
            return node.computed.width.unit != style::LengthUnit::AUTO;
        return node.computed.height.unit != style::LengthUnit::AUTO;
    }
}
