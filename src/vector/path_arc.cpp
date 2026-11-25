#include "gran/vector/internal/vector_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::vector::internal
{
    namespace
    {
        constexpr double PI = 3.14159265358979323846;
        struct Point { double x, y; };
        struct Ellipse
        {
            Point radius, start;
            double cosine, sine, sweep;
        };

        Ellipse centerForm(Vec2 from, const ArcSpec& arc)
        {
            const double degrees = std::remainder(static_cast<double>(arc.rotation), 360.0);
            const double angle = degrees * PI / 180;
            Ellipse e{{std::fabs(arc.radius.x), std::fabs(arc.radius.y)}, {}, std::cos(angle), std::sin(angle), 0};
            const double magnitude = std::fabs(degrees);
            if (magnitude == 90)
            {
                e.cosine = 0;
                e.sine = std::copysign(1.0, degrees);
            }
            else if (magnitude == 180)
            {
                e.cosine = -1;
                e.sine = 0;
            }
            else if (magnitude == 45 || magnitude == 135)
            {
                const double diagonal = std::sqrt(0.5);
                e.cosine = std::copysign(diagonal, 90 - magnitude);
                e.sine = std::copysign(diagonal, degrees);
            }
            const double dx = (static_cast<double>(from.x) - arc.end.x) / 2;
            const double dy = (static_cast<double>(from.y) - arc.end.y) / 2;
            double localX, localY;
            if (magnitude == 45 || magnitude == 135)
            {
                // Общий множитель оставляет точный ноль на диагонали даже при FMA.
                // Factor out the common scale to preserve exact diagonal cancellation with FMA.
                const double signedDy = std::copysign(1.0, e.cosine * e.sine) * dy;
                localX = e.cosine * (dx + signedDy);
                localY = e.sine * (signedDy - dx);
            }
            else
            {
                localX = e.cosine * dx + e.sine * dy;
                localY = -e.sine * dx + e.cosine * dy;
            }
            const double u = localX / e.radius.x;
            const double v = localY / e.radius.y;
            const double length = std::hypot(u, v);
            const double halfChord = std::min(1.0, length);
            const double normal = std::sqrt((1 - halfChord) * (1 + halfChord)) * (arc.large == arc.sweep ? -1 : 1);
            e.start = {(u / length) * halfChord - (v / length) * normal,
                       (v / length) * halfChord + (u / length) * normal};
            e.radius.x *= std::max(1.0, length);
            e.radius.y *= std::max(1.0, length);
            e.sweep = 2 * std::asin(halfChord);
            if (arc.large)
                e.sweep = 2 * PI - e.sweep;
            if (!arc.sweep)
                e.sweep = -e.sweep;
            return e;
        }

        Point transform(const Ellipse& e, Point v)
        {
            const double x = e.radius.x * v.x;
            const double y = e.radius.y * v.y;
            return {e.cosine * x - e.sine * y, e.sine * x + e.cosine * y};
        }

        // Смещение от начала через cos(t)-1 = -2 sin²(t/2) сохраняет малые дуги.
        // Evaluate offsets from the known start using cos(t)-1 = -2 sin²(t/2) for small arcs.
        Point endOffset(const Ellipse& e, double t)
        {
            const double h = std::sin(t / 2);
            const double c = -2 * h * h;
            const double s = std::sin(t);
            return transform(e, {e.start.x * c - e.start.y * s, e.start.x * s + e.start.y * c});
        }

        Point tangent(const Ellipse& e, double t)
        {
            const double c = std::cos(t), s = std::sin(t);
            return transform(e, {-e.start.x * s - e.start.y * c, e.start.x * c - e.start.y * s});
        }
    }

    /*
    ** Дуга: совпадающие концы — ничего, нулевой радиус — отрезок (как требует SVG). Иначе центральная
    ** форма и куски по 90° и меньше; конец последнего куска — ровно точка из данных (без накопленной ошибки).
    **
    ** An arc: equal endpoints — nothing, a zero radius — a segment (as SVG requires). Otherwise the center
    ** form and pieces of 90° or less; the end of the last piece is exactly the point from the data (without accumulated error).
    */
    void addArc(PathBuilder& builder, const ArcSpec& arc, const PathScanner& scanner)
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
        const Ellipse e = centerForm(from, arc);
        const int pieces = std::max(1, static_cast<int>(std::ceil(std::fabs(e.sweep) / (PI / 2))));
        const double step = e.sweep / pieces;
        const double k = 4.0 / 3 * std::tan(step / 4);
        Point start{from.x, from.y};
        for (int i = 0; i < pieces; ++i)
        {
            const double a = step * i, b = step * (i + 1);
            const Point first = tangent(e, a), last = tangent(e, b);
            const Point offset = endOffset(e, b);
            Point end{from.x + offset.x, from.y + offset.y};
            if (i + 1 == pieces)
                end = {arc.end.x, arc.end.y};
            Vec2 points[]{
                {pathCoordinate(start.x + k * first.x, scanner), pathCoordinate(start.y + k * first.y, scanner)},
                {pathCoordinate(end.x - k * last.x, scanner), pathCoordinate(end.y - k * last.y, scanner)},
                {pathCoordinate(end.x, scanner), pathCoordinate(end.y, scanner)}};
            addVerb(builder, Verb::CUBIC, points, 3);
            start = end;
        }
    }
}
