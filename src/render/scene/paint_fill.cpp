#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        /*
        ** Линейный градиент CSS: линия через центр под углом (0 — вверх, по часовой) длиной
        ** |w·sin θ| + |h·cos θ| — так углы блока получают крайние цвета.
        **
        ** A CSS linear gradient: a line through the center at the angle (0 — up, clockwise) of length
        ** |w·sin θ| + |h·cos θ| — so the box corners get the edge colors.
        */
        void linearGeometry(GpuInstance& instance, float angle)
        {
            float w = instance.rect[2];
            float h = instance.rect[3];
            float dx = std::sin(angle);
            float dy = -std::cos(angle);
            float half = (std::fabs(w * dx) + std::fabs(h * dy)) * 0.5f;
            float cx = instance.rect[0] + w * 0.5f;
            float cy = instance.rect[1] + h * 0.5f;

            instance.extra[0] = cx - dx * half;
            instance.extra[1] = cy - dy * half;
            instance.extra[2] = cx + dx * half;
            instance.extra[3] = cy + dy * half;
        }

        // Радиальный «farthest-corner»: окружность до дальнего угла, эллипс — √2 × дальние стороны
        // A radial "farthest-corner": a circle to the farthest corner, an ellipse — √2 × the farthest sides
        void radialGeometry(GpuInstance& instance, const style::Gradient& gradient)
        {
            float w = instance.rect[2];
            float h = instance.rect[3];
            float sideX = std::max(gradient.center.x, 1.0f - gradient.center.x) * w;
            float sideY = std::max(gradient.center.y, 1.0f - gradient.center.y) * h;

            instance.extra[0] = instance.rect[0] + gradient.center.x * w;
            instance.extra[1] = instance.rect[1] + gradient.center.y * h;
            instance.extra[2] = sideX * 1.41421356f;
            instance.extra[3] = sideY * 1.41421356f;
            if (gradient.circle)
            {
                instance.extra[2] = std::sqrt(sideX * sideX + sideY * sideY);
                instance.extra[3] = instance.extra[2];
            }
        }
    }

    // Цвет — в fill; градиент — строка рампы, вид и геометрия в координатах экземпляра
    // A color goes into fill; a gradient — the ramp row, the kind and the geometry in instance coordinates
    void applyPaint(GpuInstance& instance, const style::Paint& paint, SceneContext& context)
    {
        instance.fill = 0;
        instance.paint[2] = PAINT_SOLID;
        if (paint.kind == style::PaintKind::COLOR)
            instance.fill = colorPackPremultiplied(paint.color);
        if (paint.kind != style::PaintKind::GRADIENT)
            return;
        const style::Gradient& gradient = paint.gradient;
        instance.paint[1] = context.resources.ramps.rowFor(gradient);
        if (gradient.kind == style::GradientKind::LINEAR)
        {
            instance.paint[2] = PAINT_LINEAR;
            linearGeometry(instance, gradient.angle);
            return;
        }
        radialGeometry(instance, gradient);
        instance.paint[2] = PAINT_RADIAL;
        if (gradient.kind == style::GradientKind::CONIC)
        {
            instance.paint[2] = PAINT_CONIC;
            instance.extra[2] = gradient.angle;
        }
    }
}
