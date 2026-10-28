#ifndef GRAN_VECTOR_SHAPE_HPP
#define GRAN_VECTOR_SHAPE_HPP

#include "gran/text/glyph_rasterizer.hpp"
#include "gran/utils/geometry.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gran::vector
{
    // Шаг контура в абсолютных координатах viewBox / A contour step in absolute viewBox coordinates
    enum class Verb : std::uint8_t
    {
        MOVE,    // 1 точка: начало нового подконтура / 1 point: the start of a new subpath
        LINE,    // 1 точка: отрезок / 1 point: a segment
        QUAD,    // 2 точки: контрольная и конец / 2 points: the control and the end
        CUBIC,   // 3 точки: две контрольные и конец / 3 points: two controls and the end
        CLOSE    // 0 точек: замкнуть подконтур / 0 points: close the subpath
    };

    // Контур с правилом заливки (evenOdd — «чётно-нечётно», иначе ненулевая обмотка) / A contour with the fill rule (evenOdd — "even-odd", otherwise non-zero winding)
    struct Path
    {
        std::vector<Verb> verbs;
        std::vector<Vec2> points;
        bool evenOdd = false;
    };

    /*
    ** Векторная фигура иконки: контуры в системе координат viewBox (как в SVG: ось y вниз).
    ** id — уникальный номер фигуры в процессе (ключ растров в атласе: адрес мог бы повториться
    ** после удаления фигуры, номер — нет). Фигура не меняется после построения.
    **
    ** The icon vector shape: the contours in the viewBox coordinate system (as in SVG: the y axis down).
    ** id is the unique shape number in the process (the key of the rasters in the atlas: an address could repeat
    ** after a shape is deleted, the number cannot). The shape does not change after it is built.
    */
    struct Shape
    {
        Rect viewBox{0.0f, 0.0f, 24.0f, 24.0f};
        std::vector<Path> paths;
        std::uint32_t id = 0;
    };

    // ---- path_parser.cpp ----

    /*
    ** Данные пути SVG (атрибут d): M L H V C S Q T A Z в обоих регистрах, неявные повторы команд,
    ** числа с экспонентой и без разделителей («1.5.5», «-1-2»), сжатые флаги дуги. Дуги переводятся
    ** в кубические кривые. Ошибка — GranException(MARKUP) с позицией в строке.
    **
    ** The SVG path data (the d attribute): M L H V C S Q T A Z in both cases, implicit command repeats,
    ** numbers with an exponent and without separators ("1.5.5", "-1-2"), compact arc flags. Arcs are converted
    ** into cubic curves. An error is GranException(MARKUP) with the position in the string.
    */
    Path parsePathData(std::string_view data);

    // ---- view_box.cpp ----

    // viewBox «x y ширина высота» (разделители — пробелы и запятые); false — не так или размеры не положительны
    // viewBox "x y width height" (the separators are spaces and commas); false — not so or the sizes are not positive
    bool parseViewBox(std::string_view text, Rect& viewBox);

    // Новый уникальный номер фигуры (потокобезопасно) / A new unique shape number (thread-safe)
    std::uint32_t nextShapeId() noexcept;

    // ---- svg_loader.cpp ----

    /*
    ** Иконка из файла SVG через «Берёсту» (внешние сущности и DTD не грузятся): viewBox корня (или
    ** width/height), залитые <path>, <rect>, <circle>, <ellipse>, <polygon>, <polyline> на любой
    ** глубине <g>; fill="none" — пропуск. Обводки, преобразования и стили CSS не поддерживаются.
    ** Ошибка чтения или разбора — GranException.
    **
    ** An icon from an SVG file through Beresta (external entities and DTDs are not loaded): the root viewBox (or
    ** width/height), the filled <path>, <rect>, <circle>, <ellipse>, <polygon>, <polyline> at any
    ** <g> depth; fill="none" is skipped. Strokes, transforms and CSS styles are not supported.
    ** A read or parse error is GranException.
    */
    std::shared_ptr<const Shape> loadSvgFile(const std::string& path);

    // ---- shape_raster.cpp ----

    /*
    ** Растр фигуры точной площадью покрытия: viewBox растягивается на width × height пикселей
    ** устройства. Несколько контуров объединяются (покрытие a + b − a·b). false — фигура пуста
    ** или растр больше предела.
    **
    ** The shape raster by the exact coverage area: the viewBox is stretched over width × height device
    ** pixels. Several contours are united (the coverage a + b − a·b). false — the shape is empty
    ** or the raster exceeds the limit.
    */
    bool rasterizeShape(const Shape& shape, float width, float height, text::GlyphBitmap& bitmap);
}

#endif
