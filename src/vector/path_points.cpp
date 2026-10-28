#include "gran/vector/internal/vector_internal.hpp"

namespace gran::vector::internal
{
    // Относительная точка отсчитывается от текущей точки до начала команды / A relative point counts from the current point before the command starts
    Vec2 readPoint(const PathBuilder& builder, PathScanner& scanner, bool relative)
    {
        Vec2 point;

        point.x = requireNumber(scanner);
        point.y = requireNumber(scanner);
        if (relative)
            point = Vec2{point.x + builder.current.x, point.y + builder.current.y};
        return point;
    }

    /*
    ** Шаг в контур; последняя точка становится текущей. MOVE задаёт начало подконтура, CLOSE
    ** возвращает к нему. Для S/T запоминается контрольная точка кривой (у кубической — вторая).
    **
    ** A step into the contour; the last point becomes the current one. MOVE sets the subpath start, CLOSE
    ** returns to it. For S/T the curve control point is kept (for a cubic — the second one).
    */
    void addVerb(PathBuilder& builder, Verb verb, const Vec2* points, std::size_t count)
    {
        builder.path.verbs.push_back(verb);
        for (std::size_t index = 0; index < count; ++index)
            builder.path.points.push_back(points[index]);
        if (count > 0)
            builder.current = points[count - 1];
        if (verb == Verb::MOVE)
            builder.start = builder.current;
        if (verb == Verb::CLOSE)
            builder.current = builder.start;
        builder.lastCurve = Verb::MOVE;
        if (verb == Verb::CUBIC || verb == Verb::QUAD)
        {
            builder.lastCurve = verb;
            builder.control = points[count - 2];
        }
    }

    // Отражение прошлой контрольной точки относительно текущей / The reflection of the previous control point about the current one
    Vec2 reflectedControl(const PathBuilder& builder, Verb kind) noexcept
    {
        if (builder.lastCurve != kind)
            return builder.current;
        return Vec2{2.0f * builder.current.x - builder.control.x, 2.0f * builder.current.y - builder.control.y};
    }
}
