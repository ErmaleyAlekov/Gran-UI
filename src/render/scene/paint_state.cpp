#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        // Внутренние радиусы: внешние минус прилегающая рамка / The inner radii: the outer ones minus the adjacent border
        CornerRadii innerRadii(const style::ComputedStyle& style) noexcept
        {
            const Insets& b = style.borderWidth;
            const CornerRadii& r = style.borderRadius;

            return CornerRadii{std::max(0.0f, r.topLeft - std::max(b.left, b.top)),
                std::max(0.0f, r.topRight - std::max(b.right, b.top)),
                std::max(0.0f, r.bottomRight - std::max(b.right, b.bottom)),
                std::max(0.0f, r.bottomLeft - std::max(b.left, b.bottom))};
        }
    }

    namespace
    {
        // Прямоугольник отсечения в координатах экземпляра; в выровненном случае — пересечение с предком
        // The clip rectangle in instance coordinates; in the aligned case — the intersection with the ancestor
        Rect clipRect(const ui::Node& node, const PaintState& state, SceneContext& context, float& unit)
        {
            GpuInstance placed = placeInstance(state, rectDeflate(node.layout.frame, node.computed.borderWidth),
                context.scale, unit);
            Rect rect{placed.rect[0], placed.rect[1], placed.rect[2], placed.rect[3]};

            if (state.snapped && state.clip != 0)
            {
                const GpuClip& outer = context.scene.clips[state.clip];
                rect = rectIntersect(rect, Rect{outer.rect[0], outer.rect[1], outer.rect[2], outer.rect[3]});
            }
            return rect;
        }
    }

    /*
    ** Непрозрачность умножается вниз по дереву, преобразование узла (ui::nodeTransform) —
    ** справа к преобразованию родителя. Выравнивание по пикселям сохраняется, только если
    ** линейная часть преобразования единичная (чистый сдвиг).
    **
    ** The opacity multiplies down the tree, the node transform (ui::nodeTransform) is applied
    ** on the right of the parent transform. The pixel alignment is kept only if
    ** the linear part of the transform is the identity (a pure shift).
    */
    PaintState nodeState(const ui::Node& node, const PaintState& parent)
    {
        PaintState state = parent;

        state.opacity = parent.opacity * node.computed.opacity;
        if (node.computed.transform.empty())
            return state;
        Affine2D local = ui::nodeTransform(node);
        state.transform = affineMultiply(parent.transform, local);
        state.snapped = parent.snapped && local.a == 1.0f && local.b == 0.0f && local.c == 0.0f && local.d == 1.0f;
        return state;
    }

    /*
    ** Отсечение по внутреннему краю рамки с внутренними радиусами. В выровненном случае
    ** прямоугольник — в пикселях устройства и пересекается с отсечением предка; иначе —
    ** в логических координатах с обратным преобразованием (отсечение предка не учитывается).
    **
    ** A clip by the border inner edge with the inner radii. In the aligned case the rectangle
    ** is in device pixels and is intersected with the ancestor clip; otherwise it is
    ** in logical coordinates with the inverse transform (the ancestor clip is not taken into account).
    */
    PaintState clipChildren(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const style::ComputedStyle& style = node.computed;
        bool clips = style.overflow == static_cast<int>(style::Overflow::HIDDEN) || node.kind == ui::ElementKind::SCROLL;
        PaintState inner = state;
        float unit = 1.0f;
        Affine2D inverse;

        if (!clips)
            return inner;
        Rect rect = clipRect(node, state, context, unit);
        if (!state.snapped)
            affineInvert(state.transform, inverse);
        CornerRadii radii = fitRadii(innerRadii(style), rect.width, rect.height, unit);
        GpuClip clip{{rect.x, rect.y, rect.width, rect.height}, {radii.topLeft, radii.topRight, radii.bottomRight,
            radii.bottomLeft}, {inverse.a, inverse.b, inverse.c, inverse.d}, {inverse.tx, inverse.ty}, {0.0f, 0.0f}};
        inner.clip = static_cast<std::uint32_t>(context.scene.clips.size());
        context.scene.clips.push_back(clip);
        return inner;
    }
}
