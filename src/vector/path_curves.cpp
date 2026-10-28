#include "gran/vector/internal/vector_internal.hpp"

namespace gran::vector::internal
{
    /*
    ** Кривые: C — две контрольные и конец, S — первая контрольная отражается от прошлой C/S,
    ** Q — контрольная и конец, T — контрольная отражается от прошлой Q/T. Все относительные точки
    ** одной команды отсчитываются от точки до команды (текущая меняется только в addVerb).
    **
    ** The curves: C — two controls and the end, S — the first control is reflected from the previous C/S,
    ** Q — a control and the end, T — the control is reflected from the previous Q/T. All the relative points
    ** of one command count from the point before the command (the current one changes only in addVerb).
    */
    void runCurve(PathBuilder& builder, PathScanner& scanner, char command, bool relative)
    {
        Vec2 points[3];

        if (command == 'C')
        {
            for (Vec2& point : points)
                point = readPoint(builder, scanner, relative);
            addVerb(builder, Verb::CUBIC, points, 3);
        }
        else if (command == 'S')
        {
            points[0] = reflectedControl(builder, Verb::CUBIC);
            points[1] = readPoint(builder, scanner, relative);
            points[2] = readPoint(builder, scanner, relative);
            addVerb(builder, Verb::CUBIC, points, 3);
        }
        else
        {
            points[0] = reflectedControl(builder, Verb::QUAD);
            if (command == 'Q')
                points[0] = readPoint(builder, scanner, relative);
            points[1] = readPoint(builder, scanner, relative);
            addVerb(builder, Verb::QUAD, points, 2);
        }
    }

    // Дуга: rx ry поворот флаг-большая флаг-направление x y / An arc: rx ry rotation large-flag sweep-flag x y
    void runArc(PathBuilder& builder, PathScanner& scanner, bool relative)
    {
        ArcSpec arc;

        arc.radius.x = requireNumber(scanner);
        arc.radius.y = requireNumber(scanner);
        arc.rotation = requireNumber(scanner);
        if (!readFlag(scanner, arc.large) || !readFlag(scanner, arc.sweep))
            throwPathError(scanner);
        arc.end = readPoint(builder, scanner, relative);
        addArc(builder, arc);
    }
}
