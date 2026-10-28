#include "gran/vector/internal/vector_internal.hpp"

namespace gran::vector::internal
{
    namespace
    {
        // H и V: отрезок по одной оси, вторая координата — от текущей точки / H and V: a segment along one axis, the other coordinate is from the current point
        void axisLine(PathBuilder& builder, PathScanner& scanner, char command, bool relative)
        {
            Vec2 point = builder.current;
            float value = requireNumber(scanner);

            if (command == 'H' && relative)
                point.x += value;
            else if (command == 'H')
                point.x = value;
            else if (relative)
                point.y += value;
            else
                point.y = value;
            addVerb(builder, Verb::LINE, &point, 1);
        }
    }

    /*
    ** Команда по заглавной букве. Контур обязан начинаться с M: иначе текущей точки нет.
    ** C, S, Q, T — кривые, A — дуга; неизвестная буква — ошибка в её позиции.
    **
    ** A command by the capital letter. A contour must start with M: otherwise there is no current point.
    ** C, S, Q, T are curves, A is an arc; an unknown letter is an error at its position.
    */
    void runCommand(PathBuilder& builder, PathScanner& scanner, char command, bool relative)
    {
        Vec2 point;

        if (builder.path.verbs.empty() && command != 'M')
            throwPathError(scanner);
        if (command == 'M' || command == 'L')
        {
            point = readPoint(builder, scanner, relative);
            if (command == 'M')
                addVerb(builder, Verb::MOVE, &point, 1);
            else
                addVerb(builder, Verb::LINE, &point, 1);
        }
        else if (command == 'H' || command == 'V')
            axisLine(builder, scanner, command, relative);
        else if (command == 'Z')
            addVerb(builder, Verb::CLOSE, nullptr, 0);
        else if (command == 'C' || command == 'S' || command == 'Q' || command == 'T')
            runCurve(builder, scanner, command, relative);
        else if (command == 'A')
            runArc(builder, scanner, relative);
        else
            throwPathError(scanner);
    }
}
