#include "gran/vector/internal/vector_internal.hpp"

#include <algorithm>

namespace gran::vector::internal
{
    namespace
    {
        // Доля радиуса для контрольных точек четверти эллипса кубической кривой / The radius share for the control points of an ellipse quarter as a cubic curve
        constexpr float KAPPA = 0.5522847498f;

        // Отрезок до from, затем четверть эллипса через угол corner к to / A segment to from, then an ellipse quarter through the corner corner to to
        void appendCorner(PathBuilder& builder, Vec2 from, Vec2 corner, Vec2 to)
        {
            Vec2 points[3] = {
                Vec2{from.x + KAPPA * (corner.x - from.x), from.y + KAPPA * (corner.y - from.y)},
                Vec2{to.x + KAPPA * (corner.x - to.x), to.y + KAPPA * (corner.y - to.y)},
                to};

            addVerb(builder, Verb::LINE, &from, 1);
            addVerb(builder, Verb::CUBIC, points, 3);
        }
    }

    /*
    ** <rect>: обход по часовой стрелке от верхней стороны; радиусы ограничены половиной сторон
    ** (как в SVG). Нулевой радиус — те же углы, только четверть эллипса вырождается в точку.
    **
    ** <rect>: a clockwise walk from the top side; the radii are limited by half the sides
    ** (as in SVG). A zero radius gives the same corners, only the ellipse quarter degenerates into a point.
    */
    Path rectPath(const Rect& rect, Vec2 radius)
    {
        PathBuilder builder;
        float rx = std::min(std::max(0.0f, radius.x), rect.width * 0.5f);
        float ry = std::min(std::max(0.0f, radius.y), rect.height * 0.5f);
        float right = rect.x + rect.width;
        float bottom = rect.y + rect.height;
        Vec2 start{rect.x + rx, rect.y};

        if (rect.width <= 0.0f || rect.height <= 0.0f)
            return builder.path;
        addVerb(builder, Verb::MOVE, &start, 1);
        appendCorner(builder, Vec2{right - rx, rect.y}, Vec2{right, rect.y}, Vec2{right, rect.y + ry});
        appendCorner(builder, Vec2{right, bottom - ry}, Vec2{right, bottom}, Vec2{right - rx, bottom});
        appendCorner(builder, Vec2{rect.x + rx, bottom}, Vec2{rect.x, bottom}, Vec2{rect.x, bottom - ry});
        appendCorner(builder, Vec2{rect.x, rect.y + ry}, Vec2{rect.x, rect.y}, start);
        addVerb(builder, Verb::CLOSE, nullptr, 0);
        return builder.path;
    }

    // <circle>/<ellipse>: четыре четверти от правой точки / <circle>/<ellipse>: four quarters from the right point
    Path ellipsePath(Vec2 center, Vec2 radius)
    {
        PathBuilder builder;
        Vec2 right{center.x + radius.x, center.y};

        if (radius.x <= 0.0f || radius.y <= 0.0f)
            return builder.path;
        addVerb(builder, Verb::MOVE, &right, 1);
        appendCorner(builder, right, Vec2{right.x, center.y + radius.y}, Vec2{center.x, center.y + radius.y});
        appendCorner(builder, builder.current, Vec2{center.x - radius.x, center.y + radius.y}, Vec2{center.x - radius.x, center.y});
        appendCorner(builder, builder.current, Vec2{center.x - radius.x, center.y - radius.y}, Vec2{center.x, center.y - radius.y});
        appendCorner(builder, builder.current, Vec2{right.x, center.y - radius.y}, right);
        addVerb(builder, Verb::CLOSE, nullptr, 0);
        return builder.path;
    }

    // <polygon>/<polyline>: пары чисел; непарное последнее число отбрасывается (как в браузерах) / Number pairs; an unpaired last number is dropped (as in browsers)
    Path polylinePath(std::string_view points, bool closed)
    {
        PathBuilder builder;
        PathScanner scanner{points, 0};
        Vec2 point;

        while (readNumber(scanner, point.x) && readNumber(scanner, point.y))
        {
            if (builder.path.verbs.empty())
                addVerb(builder, Verb::MOVE, &point, 1);
            else
                addVerb(builder, Verb::LINE, &point, 1);
        }
        if (closed && !builder.path.verbs.empty())
            addVerb(builder, Verb::CLOSE, nullptr, 0);
        return builder.path;
    }
}
