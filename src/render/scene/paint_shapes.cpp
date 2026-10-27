#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    // Радиус не больше половины меньшей стороны (иначе SDF-прямоугольник искажается) / The radius is not above half the smaller side (otherwise the SDF rectangle distorts)
    void emitBox(const ControlBox& box, float shadow, const PaintState& state, SceneContext& context)
    {
        float unit = 1.0f;
        GpuInstance instance = placeInstance(state, box.rect, context.scale, unit);

        for (float& radius : instance.radii)
            radius = std::min(box.radius * unit, std::min(instance.rect[2], instance.rect[3]) * 0.5f);
        if (shadow > 0.0f)
        {
            GpuInstance below = instance;
            below.kind = KIND_SHADOW;
            below.param = shadow * unit;
            below.extra[1] = shadow * 0.5f * unit;
            below.fill = colorPackPremultiplied(Color{0.0f, 0.0f, 0.0f, 0.3f});
            context.scene.instances.push_back(below);
        }
        instance.kind = KIND_RECT;
        instance.fill = colorPackPremultiplied(box.fill);
        for (float& side : instance.border)
            side = box.border * unit;
        instance.stroke = colorPackPremultiplied(box.stroke);
        context.scene.instances.push_back(instance);
    }

    // Полоса — прямоугольник со скруглёнными концами, повёрнутый вокруг своей середины / A bar is a rectangle with round ends rotated around its middle
    void emitBar(const Bar& line, const Color& color, const PaintState& state, SceneContext& context)
    {
        float dx = line.to.x - line.from.x;
        float dy = line.to.y - line.from.y;
        float length = std::sqrt(dx * dx + dy * dy) + line.thickness;
        float angle = std::atan2(dy, dx);
        PaintState bar = state;
        Affine2D place{std::cos(angle), std::sin(angle), -std::sin(angle), std::cos(angle),
            (line.from.x + line.to.x) * 0.5f, (line.from.y + line.to.y) * 0.5f};

        bar.transform = affineMultiply(state.transform, place);
        bar.snapped = false;
        emitBox(ControlBox{Rect{-length * 0.5f, -line.thickness * 0.5f, length, line.thickness}, line.thickness * 0.5f,
            color, color, 0.0f}, 0.0f, bar, context);
    }

    /*
    ** Галочка-стрелка из двух полос, сходящихся в вершине: вниз (direction = (0, 1)) — у ComboBox,
    ** вправо (1, 0) — у пункта с подменю. size — половина размаха стрелки.
    **
    ** A chevron arrow of two bars meeting at the tip: down (direction = (0, 1)) — for a ComboBox,
    ** right (1, 0) — for an item with a submenu. size is half the arrow span.
    */
    void emitChevron(const Chevron& mark, const PaintState& state, SceneContext& context)
    {
        Vec2 axis = mark.direction;
        float half = mark.size * 0.5f;
        Vec2 tip{mark.center.x + axis.x * half, mark.center.y + axis.y * half};
        Vec2 back{mark.center.x - axis.x * half, mark.center.y - axis.y * half};
        Vec2 side{-axis.y * mark.size, axis.x * mark.size};
        float thickness = std::fmax(1.25f, mark.size * 0.32f);

        emitBar(Bar{Vec2{back.x + side.x, back.y + side.y}, tip, thickness}, mark.color, state, context);
        emitBar(Bar{Vec2{back.x - side.x, back.y - side.y}, tip, thickness}, mark.color, state, context);
    }
}
