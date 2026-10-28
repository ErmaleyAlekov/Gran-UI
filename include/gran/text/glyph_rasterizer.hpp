#ifndef GRAN_TEXT_GLYPH_RASTERIZER_HPP
#define GRAN_TEXT_GLYPH_RASTERIZER_HPP

#include "gran/text/font_face.hpp"

#include <cstdint>
#include <vector>

namespace gran::text
{
    /*
    ** Растр глифа: покрытие 0..255 построчно сверху вниз. left и top — смещение левого
    ** верхнего угла растра от точки пера на базовой линии (ось y вниз, top обычно < 0).
    **
    ** A glyph raster: the coverage 0..255 row by row top to bottom. left and top are the offset
    ** of the raster top left corner from the pen point on the baseline (the y axis down, top is usually < 0).
    */
    struct GlyphBitmap
    {
        int width = 0;
        int height = 0;
        int left = 0;
        int top = 0;
        std::vector<std::uint8_t> coverage;
    };

    /*
    ** Растеризация контура точной площадью покрытия (как в браузерах без хинтинга).
    ** toPixels переводит единицы шрифта (ось y вверх) в пиксели (ось y вниз): масштаб,
    ** наклон для курсива, подпиксельный сдвиг. false — контур пуст (пробел).
    **
    ** Rasterizing an outline by the exact coverage area (as browsers do without hinting).
    ** toPixels turns font units (the y axis up) into pixels (the y axis down): the scale,
    ** the slant for italic, the subpixel shift. false — the outline is empty (a space).
    */
    bool rasterizeOutline(const Outline& outline, const Affine2D& toPixels, GlyphBitmap& bitmap);

    /*
    ** Накопитель площади: каждый отрезок контура добавляет в ячейки строк знаковую долю
    ** площади; сумма слева направо даёт покрытие пикселя (правило ненулевой обмотки с насыщением).
    **
    ** An area accumulator: each outline segment adds a signed area share into the row cells;
    ** the left to right sum gives the pixel coverage (the non-zero winding rule with saturation).
    */
    class CoverageAccumulator
    {
    public:
        // ---- raster_accumulator.cpp ----
        CoverageAccumulator(int width, int height);

        // Отрезок в пиксельных координатах растра / A segment in raster pixel coordinates
        void addLine(Vec2 from, Vec2 to);

        // Квадратичная кривая: разбивается на отрезки по кривизне / A quadratic curve: split into segments by curvature
        void addQuadratic(Vec2 from, Vec2 control, Vec2 to);

        // Покрытие 0..255; evenOdd — правило «чётно-нечётно» (иконки SVG), иначе ненулевая обмотка
        // The coverage 0..255; evenOdd — the "even-odd" rule (SVG icons), otherwise non-zero winding
        void resolve(std::vector<std::uint8_t>& coverage, bool evenOdd = false) const;

    private:
        // ---- raster_cells.cpp ----
        // Вклад отрезка в одну строку / The segment contribution to one row
        void addRowSpan(std::size_t rowStart, float x, float xNext, float delta);

        int _width;
        int _height;
        std::vector<float> _cells;
    };
}

#endif
