#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        // Края блока округляются до пикселей устройства по отдельности, преобразование — единичное
        // The box edges are rounded to device pixels separately, the transform is the identity
        void placeSnapped(GpuInstance& instance, const Affine2D& t, const Rect& logical)
        {
            float left = std::round(logical.x * t.a + t.tx);
            float top = std::round(logical.y * t.d + t.ty);
            float right = std::round((logical.x + logical.width) * t.a + t.tx);
            float bottom = std::round((logical.y + logical.height) * t.d + t.ty);

            instance.rect[0] = left;
            instance.rect[1] = top;
            instance.rect[2] = right - left;
            instance.rect[3] = bottom - top;
            instance.transform[0] = 1.0f;
            instance.transform[3] = 1.0f;
        }
    }

    // Выровненный или повёрнутый/масштабированный случай / The aligned or the rotated/scaled case
    GpuInstance placeInstance(const PaintState& state, const Rect& logical, float scale, float& unit)
    {
        GpuInstance instance{};
        const Affine2D& t = state.transform;

        instance.opacity = state.opacity;
        instance.clip = state.clip;
        if (state.snapped)
        {
            placeSnapped(instance, t, logical);
            unit = scale;
            return instance;
        }
        instance.rect[0] = logical.x;
        instance.rect[1] = logical.y;
        instance.rect[2] = logical.width;
        instance.rect[3] = logical.height;
        instance.transform[0] = t.a;
        instance.transform[1] = t.b;
        instance.transform[2] = t.c;
        instance.transform[3] = t.d;
        instance.translate[0] = t.tx;
        instance.translate[1] = t.ty;
        unit = 1.0f;
        return instance;
    }

    /*
    ** CSS: если сумма радиусов на стороне больше её длины, все радиусы уменьшаются
    ** одним множителем — наименьшим отношением «сторона / сумма».
    **
    ** CSS: if the radius sum on a side exceeds its length, all radii are reduced
    ** by one factor — the smallest "side / sum" ratio.
    */
    CornerRadii fitRadii(const CornerRadii& radii, float width, float height, float unit) noexcept
    {
        CornerRadii scaled{radii.topLeft * unit, radii.topRight * unit, radii.bottomRight * unit, radii.bottomLeft * unit};
        float factor = 1.0f;
        float sums[4] = {scaled.topLeft + scaled.topRight, scaled.bottomLeft + scaled.bottomRight,
            scaled.topLeft + scaled.bottomLeft, scaled.topRight + scaled.bottomRight};
        float sides[4] = {width, width, height, height};

        for (int index = 0; index < 4; ++index)
        {
            if (sums[index] > sides[index] && sums[index] > 0.0f)
                factor = std::min(factor, sides[index] / sums[index]);
        }
        return CornerRadii{scaled.topLeft * factor, scaled.topRight * factor, scaled.bottomRight * factor,
            scaled.bottomLeft * factor};
    }

    // Рамка минус padding и border / The frame minus padding and border
    Rect contentRect(const ui::Node& node) noexcept
    {
        const style::ComputedStyle& style = node.computed;
        Insets insets{style.padding.top + style.borderWidth.top, style.padding.right + style.borderWidth.right,
            style.padding.bottom + style.borderWidth.bottom, style.padding.left + style.borderWidth.left};

        return rectDeflate(node.layout.frame, insets);
    }
}
