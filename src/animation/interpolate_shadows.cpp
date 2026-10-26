#include "gran/animation/internal/interpolation.hpp"

#include <algorithm>

namespace gran::animation::internal
{
    namespace
    {
        // Тень из списка или прозрачная копия тени другой стороны / A shadow from the list or a transparent copy of the other side's shadow
        style::Shadow shadowOrClear(const std::vector<style::Shadow>& own, const std::vector<style::Shadow>& other,
            std::size_t index)
        {
            if (index < own.size())
                return own[index];
            style::Shadow clear = other[index];
            clear.color.a = 0.0f;
            return clear;
        }
    }

    // Пары теней; inset берётся у конца / The shadow pairs; inset is taken from the end
    std::vector<style::Shadow> mixShadows(const std::vector<style::Shadow>& from, const std::vector<style::Shadow>& to, float k)
    {
        std::vector<style::Shadow> result;
        std::size_t count = std::max(from.size(), to.size());

        for (std::size_t index = 0; index < count; ++index)
        {
            style::Shadow a = shadowOrClear(from, to, index);
            style::Shadow b = shadowOrClear(to, from, index);
            result.push_back(style::Shadow{mixNumber(a.offsetX, b.offsetX, k), mixNumber(a.offsetY, b.offsetY, k),
                mixNumber(a.blur, b.blur, k), mixNumber(a.spread, b.spread, k), mixColor(a.color, b.color, k), b.inset});
        }
        return result;
    }

    // «Нет» участвует как прозрачный цвет той же тональности / "None" takes part as a transparent color of the same hue
    style::Paint mixPaint(const style::Paint& from, const style::Paint& to, float k)
    {
        style::Paint start = from;
        style::Paint end = to;
        style::Paint result = to;

        if (start.kind == style::PaintKind::NONE && end.kind == style::PaintKind::COLOR)
            start = style::Paint{style::PaintKind::COLOR, Color{end.color.r, end.color.g, end.color.b, 0.0f}, {}};
        if (end.kind == style::PaintKind::NONE && start.kind == style::PaintKind::COLOR)
            end = style::Paint{style::PaintKind::COLOR, Color{start.color.r, start.color.g, start.color.b, 0.0f}, {}};
        if (start.kind != style::PaintKind::COLOR || end.kind != style::PaintKind::COLOR)
            return result;
        result.kind = style::PaintKind::COLOR;
        result.color = mixColor(start.color, end.color, k);
        return result;
    }
}
