#ifndef GRAN_VECTOR_INTERNAL_VECTOR_INTERNAL_HPP
#define GRAN_VECTOR_INTERNAL_VECTOR_INTERNAL_HPP

#include "gran/vector/shape.hpp"

namespace gran::vector::internal
{
    // Чтение чисел и флагов из строки данных пути / Reading numbers and flags from a path data string
    struct PathScanner
    {
        std::string_view text;
        std::size_t pos = 0;
    };

    /*
    ** Построение контура: текущая точка, начало подконтура, отражаемая контрольная точка
    ** прошлой кривой (для S и T) и её вид.
    **
    ** Building a contour: the current point, the subpath start, the reflectable control point
    ** of the previous curve (for S and T) and its kind.
    */
    struct PathBuilder
    {
        Path path;
        Vec2 current;
        Vec2 start;
        Vec2 control;
        Verb lastCurve = Verb::MOVE;
    };

    // Параметры дуги SVG (A): радиусы, поворот оси x в градусах, флаги, конец / The SVG arc parameters (A): the radii, the x axis rotation in degrees, the flags, the end
    struct ArcSpec
    {
        Vec2 radius;
        float rotation = 0.0f;
        bool large = false;
        bool sweep = false;
        Vec2 end;
    };

    // ---- path_scanner.cpp ----

    // Пропустить пробелы и одну запятую / Skip the spaces and one comma
    void skipSeparators(PathScanner& scanner) noexcept;

    // Число SVG; false — числа нет / An SVG number; false — no number
    bool readNumber(PathScanner& scanner, float& value);

    // Ошибка данных пути в текущей позиции → GranException(MARKUP) / A path data error at the current position → GranException(MARKUP)
    [[noreturn]] void throwPathError(const PathScanner& scanner);

    // ---- path_tokens.cpp ----

    // Флаг дуги «0» или «1» (может стоять вплотную к следующему) / An arc flag "0" or "1" (it may stand right against the next one)
    bool readFlag(PathScanner& scanner, bool& value) noexcept;

    // Дальше число (а не буква команды и не конец) / A number follows (not a command letter and not the end)
    bool numberAhead(PathScanner& scanner) noexcept;

    // Число, обязательное по грамматике; нет — ошибка / A number required by the grammar; none — an error
    float requireNumber(PathScanner& scanner);

    // ---- path_points.cpp ----

    // Точка «x y»; relative — от текущей точки / A point "x y"; relative — from the current point
    Vec2 readPoint(const PathBuilder& builder, PathScanner& scanner, bool relative);

    // Шаг контура с точками / A contour step with points
    void addVerb(PathBuilder& builder, Verb verb, const Vec2* points, std::size_t count);

    // Отражённая контрольная точка для S/T: прошлая кривая того же вида — отражение, иначе текущая точка
    // The reflected control point for S/T: a previous curve of the same kind — the reflection, otherwise the current point
    Vec2 reflectedControl(const PathBuilder& builder, Verb kind) noexcept;

    // ---- path_commands.cpp ----

    // Одна команда с её числами (буква уже прочитана; relative — строчная) / One command with its numbers (the letter is already read; relative — lowercase)
    void runCommand(PathBuilder& builder, PathScanner& scanner, char command, bool relative);

    // ---- path_curves.cpp ----

    // C/S и Q/T: читает точки, добавляет кривую / C/S and Q/T: reads the points, adds the curve
    void runCurve(PathBuilder& builder, PathScanner& scanner, char command, bool relative);

    // A: читает параметры дуги и добавляет её кубическими кривыми / A: reads the arc parameters and adds it as cubic curves
    void runArc(PathBuilder& builder, PathScanner& scanner, bool relative);

    // ---- path_arc.cpp ----

    // Дуга от текущей точки в кубические кривые (SVG 1.1, приложение F.6) / An arc from the current point into cubic curves (SVG 1.1, appendix F.6)
    void addArc(PathBuilder& builder, const ArcSpec& arc);

    // ---- svg_elements.cpp ----

    // Прямоугольник (со скруглением rx/ry) в контур / A rectangle (with rx/ry rounding) into a contour
    Path rectPath(const Rect& rect, Vec2 radius);

    // Эллипс в контур из четырёх кубических кривых / An ellipse into a contour of four cubic curves
    Path ellipsePath(Vec2 center, Vec2 radius);

    // Ломаная из списка «x,y x,y …»; closed — многоугольник / A polyline from the list "x,y x,y …"; closed — a polygon
    Path polylinePath(std::string_view points, bool closed);
}

#endif
