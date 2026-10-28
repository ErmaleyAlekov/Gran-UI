#include "gran/vector/internal/vector_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::vector::internal
{
    namespace
    {
        constexpr float PI = 3.14159265358979f;

        // Эллипс дуги в центральной форме: центр, радиусы, поворот, начальный угол и размах
        // The arc ellipse in the center form: the center, the radii, the rotation, the start angle and the sweep
        struct ArcEllipse
        {
            Vec2 center;
            Vec2 radius;
            float cosine = 1.0f;
            float sine = 0.0f;
            float start = 0.0f;
            float sweep = 0.0f;
        };

        // Точка единичной окружности → точка повёрнутого эллипса / A unit circle point → a point of the rotated ellipse
        Vec2 mapUnit(const ArcEllipse& ellipse, float x, float y) noexcept
        {
            float ex = ellipse.radius.x * x;
            float ey = ellipse.radius.y * y;

            return Vec2{ellipse.center.x + ex * ellipse.cosine - ey * ellipse.sine,
                ellipse.center.y + ex * ellipse.sine + ey * ellipse.cosine};
        }

        /*
        ** Переход от концов к центру (SVG 1.1, F.6.5–F.6.6): точки в системе эллипса, слишком
        ** малые радиусы растягиваются (λ > 1), центр по флагам, углы через atan2 — без acos и его
        ** потери точности на краях.
        **
        ** The conversion from the endpoints to the center (SVG 1.1, F.6.5–F.6.6): the points in the ellipse system, too
        ** small radii are stretched (λ > 1), the center by the flags, the angles via atan2 — without acos and its
        ** precision loss at the edges.
        */
        ArcEllipse centerForm(Vec2 from, const ArcSpec& arc, Vec2 radius)
        {
            ArcEllipse ellipse;
            float angle = arc.rotation * PI / 180.0f;
            ellipse.cosine = std::cos(angle);
            ellipse.sine = std::sin(angle);
            float dx = (from.x - arc.end.x) * 0.5f;
            float dy = (from.y - arc.end.y) * 0.5f;
            float x1 = ellipse.cosine * dx + ellipse.sine * dy;
            float y1 = -ellipse.sine * dx + ellipse.cosine * dy;
            float lambda = (x1 * x1) / (radius.x * radius.x) + (y1 * y1) / (radius.y * radius.y);
            if (lambda > 1.0f)
                radius = Vec2{radius.x * std::sqrt(lambda), radius.y * std::sqrt(lambda)};
            float rx2 = radius.x * radius.x;
            float ry2 = radius.y * radius.y;
            float factor = std::sqrt(std::max(0.0f, (rx2 * ry2 - rx2 * y1 * y1 - ry2 * x1 * x1) / (rx2 * y1 * y1 + ry2 * x1 * x1)));
            if (arc.large == arc.sweep)
                factor = -factor;
            float cx = factor * radius.x * y1 / radius.y;
            float cy = -factor * radius.y * x1 / radius.x;
            ellipse.center = Vec2{ellipse.cosine * cx - ellipse.sine * cy + (from.x + arc.end.x) * 0.5f,
                ellipse.sine * cx + ellipse.cosine * cy + (from.y + arc.end.y) * 0.5f};
            ellipse.radius = radius;
            ellipse.start = std::atan2((y1 - cy) / radius.y, (x1 - cx) / radius.x);
            ellipse.sweep = std::atan2((-y1 - cy) / radius.y, (-x1 - cx) / radius.x) - ellipse.start;
            return ellipse;
        }

        // Направление обхода по флагу sweep: размах в (−2π, 0] или [0, 2π) / The direction by the sweep flag: the sweep in (−2π, 0] or [0, 2π)
        void orientSweep(ArcEllipse& ellipse, bool sweep) noexcept
        {
            if (!sweep && ellipse.sweep > 0.0f)
                ellipse.sweep -= 2.0f * PI;
            if (sweep && ellipse.sweep < 0.0f)
                ellipse.sweep += 2.0f * PI;
        }

        // Кусок дуги не больше 90° одной кубической кривой (k = 4/3 · tg(Δ/4)) / An arc piece of at most 90° as one cubic curve (k = 4/3 · tan(Δ/4))
        void arcSegment(PathBuilder& builder, const ArcEllipse& ellipse, float from, float delta, Vec2 end)
        {
            float k = 4.0f / 3.0f * std::tan(delta * 0.25f);
            float to = from + delta;
            Vec2 points[3] = {
                mapUnit(ellipse, std::cos(from) - k * std::sin(from), std::sin(from) + k * std::cos(from)),
                mapUnit(ellipse, std::cos(to) + k * std::sin(to), std::sin(to) - k * std::cos(to)),
                end};

            addVerb(builder, Verb::CUBIC, points, 3);
        }
    }

    /*
    ** Дуга: совпадающие концы — ничего, нулевой радиус — отрезок (как требует SVG). Иначе центральная
    ** форма и куски по 90° и меньше; конец последнего куска — ровно точка из данных (без накопленной ошибки).
    **
    ** An arc: equal endpoints — nothing, a zero radius — a segment (as SVG requires). Otherwise the center
    ** form and pieces of 90° or less; the end of the last piece is exactly the point from the data (without accumulated error).
    */
    void addArc(PathBuilder& builder, const ArcSpec& arc)
    {
        Vec2 radius{std::fabs(arc.radius.x), std::fabs(arc.radius.y)};
        Vec2 from = builder.current;

        if (from.x == arc.end.x && from.y == arc.end.y)
            return;
        if (radius.x == 0.0f || radius.y == 0.0f)
        {
            addVerb(builder, Verb::LINE, &arc.end, 1);
            return;
        }
        ArcEllipse ellipse = centerForm(from, arc, radius);
        orientSweep(ellipse, arc.sweep);
        int pieces = std::max(1, static_cast<int>(std::ceil(std::fabs(ellipse.sweep) / (PI * 0.5f) - 1e-4f)));
        float delta = ellipse.sweep / static_cast<float>(pieces);
        for (int piece = 0; piece < pieces; ++piece)
        {
            float start = ellipse.start + delta * static_cast<float>(piece);
            Vec2 end = mapUnit(ellipse, std::cos(start + delta), std::sin(start + delta));
            if (piece == pieces - 1)
                end = arc.end;
            arcSegment(builder, ellipse, start, delta, end);
        }
    }
}
