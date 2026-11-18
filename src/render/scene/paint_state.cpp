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
        // Отсечение задано тем же обратным преобразованием (точное равенство: одно и то же значение состояния) / The clip has the same inverse transform (exact equality: one and the same state value)
        bool sameInverse(const GpuClip& clip, const Affine2D& inverse) noexcept
        {
            return clip.inverse[0] == inverse.a && clip.inverse[1] == inverse.b && clip.inverse[2] == inverse.c
                && clip.inverse[3] == inverse.d && clip.inverseTranslate[0] == inverse.tx && clip.inverseTranslate[1] == inverse.ty;
        }
    }

    /*
    ** Новое отсечение в сцене: логический прямоугольник area с радиусами radii. В выровненном случае
    ** прямоугольник — в пикселях устройства; иначе — в логических координатах с обратным преобразованием.
    **
    ** A new clip in the scene: the logical rectangle area with the radii. In the aligned case the rectangle
    ** is in device pixels; otherwise it is in logical coordinates with the inverse transform.
    */
    PaintState pushClip(const Rect& area, const CornerRadii& radii, const PaintState& state, SceneContext& context)
    {
        PaintState inner = state;
        float unit = 1.0f;
        Affine2D inverse;
        GpuInstance placed = placeInstance(state, area, context.scale, unit);
        Rect rect{placed.rect[0], placed.rect[1], placed.rect[2], placed.rect[3]};

        if (!state.snapped)
            affineInvert(state.transform, inverse);
        // Отсечение предка учитывается, когда прямоугольники в одних координатах: выровненный случай (пиксели
        // устройства) или то же обратное преобразование (предок и узел под одним поворотом) — иначе незакреплённые
        // ячейки повёрнутой таблицы вылезали бы за неё (ревью Т-66)
        // The ancestor clip is taken into account when the rectangles are in the same coordinates: the aligned case (device
        // pixels) or the same inverse transform (the ancestor and the node under one rotation) — otherwise the non-frozen
        // cells of a rotated table would stick out of it (review T-66)
        if (state.clip != 0 && (state.snapped || sameInverse(context.scene.clips[state.clip], inverse)))
        {
            const GpuClip& outer = context.scene.clips[state.clip];
            rect = rectIntersect(rect, Rect{outer.rect[0], outer.rect[1], outer.rect[2], outer.rect[3]});
        }
        CornerRadii fitted = fitRadii(radii, rect.width, rect.height, unit);
        GpuClip clip{{rect.x, rect.y, rect.width, rect.height}, {fitted.topLeft, fitted.topRight, fitted.bottomRight,
            fitted.bottomLeft}, {inverse.a, inverse.b, inverse.c, inverse.d}, {inverse.tx, inverse.ty}, {0.0f, 0.0f}};
        inner.clip = static_cast<std::uint32_t>(context.scene.clips.size());
        context.scene.clips.push_back(clip);
        return inner;
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

    // Отсечение по внутреннему краю рамки с внутренними радиусами (overflow: hidden, Scroll) / A clip by the border inner edge with the inner radii (overflow: hidden, Scroll)
    PaintState clipChildren(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const style::ComputedStyle& style = node.computed;
        bool clips = style.overflow == static_cast<int>(style::Overflow::HIDDEN) || node.kind == ui::ElementKind::SCROLL;

        if (!clips)
            return state;
        return pushClip(rectDeflate(node.layout.frame, style.borderWidth), innerRadii(style), state, context);
    }
}
