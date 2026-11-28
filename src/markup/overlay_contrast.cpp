#include "gran/markup/internal/overlay.hpp"

#include <algorithm>
#include <cmath>

namespace gran::markup::internal
{
    namespace
    {
        // Переводим каналы sRGB в линейную яркость.
        // Convert sRGB channels to linear luminance.
        double luminance(double red, double green, double blue)
        {
            const auto linear = [](double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
            return 0.2126 * linear(red) + 0.7152 * linear(green) + 0.0722 * linear(blue);
        }

        Color lowerChannels(Color a, Color b)
        {
            return {std::min(a.r,b.r), std::min(a.g,b.g), std::min(a.b,b.b), 1};
        }

        // Яркость выпукла вдоль отрезка смешения; ограничиваем минимум снизу.
        // Luminance is convex along a blend segment. Bound its minimum from below.
        double segmentMinimum(Color a, Color b)
        {
            const Color low = lowerChannels(a, b);
            if ((a.r == low.r && a.g == low.g && a.b == low.b)
                || (b.r == low.r && b.g == low.g && b.b == low.b)) return luminance(low.r, low.g, low.b);
            const auto bound = [a, b](double left, double right)
            {
                const auto channel = [left, right](double start, double end)
                {
                    return start + (end - start) * (end >= start ? left : right);
                };
                return luminance(channel(a.r, b.r), channel(a.g, b.g), channel(a.b, b.b));
            };
            double left = 0, right = 1;
            for (int i = 0; i < 44; ++i)
            {
                const double x = (2 * left + right) / 3, y = (left + 2 * right) / 3;
                if (bound(x, x) < bound(y, y))
                    right = y;
                else
                    left = x;
            }
            return bound(left, right);
        }
    }

    // Максимум яркости приходится на стоп; минимум возможен между соседними стопами.
    // Maximum luminance occurs at a stop; the minimum may lie between adjacent stops.
    void designBackdrop(const style::Paint& fill, float opacity, DesignBackdrop& backdrop)
    {
        if (fill.kind == style::PaintKind::NONE || opacity == 0) return;
        const auto& stops = fill.gradient.stops;
        const bool gradient = fill.kind == style::PaintKind::GRADIENT && !stops.empty();
        if (gradient ? std::all_of(stops.begin(), stops.end(), [](const auto& stop) { return stop.color.a == 0; })
                     : fill.color.a == 0) return;
        DesignBackdrop next;
        next.light = {0, 0, 0, 1};
        next.maximum = 0;
        Color previous{};
        for (std::size_t i = 0; i < (gradient ? stops.size() : 1); ++i)
        {
            const auto c = gradient ? stops[i].color : fill.color;
            const float alpha = c.a * opacity;
            const auto low = alpha == 1 ? c : colorMix(backdrop.dark, c, alpha);
            const auto high = alpha == 1 ? c : colorMix(backdrop.light, c, alpha);
            next.dark = lowerChannels(next.dark, low);
            next.light = {std::max(next.light.r,high.r), std::max(next.light.g,high.g), std::max(next.light.b,high.b), 1};
            next.minimum = std::min(next.minimum, i ? segmentMinimum(previous, low) : luminance(low.r, low.g, low.b));
            next.maximum = std::max(next.maximum, luminance(high.r, high.g, high.b));
            previous = low;
        }
        backdrop = next;
    }

    // Пересечение интервалов даёт гарантированный минимум, равный единице.
    // Overlapping luminance intervals guarantee only unit contrast.
    double designContrast(Color text, float opacity, const DesignBackdrop& backdrop)
    {
        const float alpha = text.a * opacity;
        const Color low = alpha == 1 ? text : colorMix(backdrop.dark, text, alpha);
        const Color high = alpha == 1 ? text : colorMix(backdrop.light, text, alpha);
        const double fgLow = luminance(low.r, low.g, low.b), fgHigh = luminance(high.r, high.g, high.b);
        if (fgLow > backdrop.maximum) return (fgLow + 0.05) / (backdrop.maximum + 0.05);
        if (fgHigh < backdrop.minimum) return (backdrop.minimum + 0.05) / (fgHigh + 0.05);
        return 1;
    }
}
