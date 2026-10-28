#include "gran/text/glyph_rasterizer.hpp"

#include <algorithm>
#include <cmath>

namespace gran::text
{
    // Ячейки обнулены; лишний столбец справа принимает «перелив» последнего пикселя строки
    // The cells are zeroed; an extra column on the right takes the "overflow" of the last pixel of a row
    CoverageAccumulator::CoverageAccumulator(int width, int height)
        : _width(width), _height(height), _cells(static_cast<std::size_t>(width) * height + 2, 0.0f)
    {
    }

    /*
    ** Отрезок ориентируется сверху вниз (знак — направление обхода), обрезается по высоте
    ** растра, затем по каждой пересекаемой строке добавляет свою площадь. X ограничивается
    ** растром: выход за ячейки невозможен даже на испорченном контуре.
    **
    ** The segment is oriented top to bottom (the sign is the winding direction), clipped by the raster
    ** height, then for each crossed row it adds its area. X is clamped to the raster: going beyond
    ** the cells is impossible even with a damaged outline.
    */
    void CoverageAccumulator::addLine(Vec2 from, Vec2 to)
    {
        float direction = 1.0f;
        float limit = static_cast<float>(_width) - 1.001f;

        if (from.y == to.y)
            return;
        if (from.y > to.y)
        {
            std::swap(from, to);
            direction = -1.0f;
        }
        float slope = (to.x - from.x) / (to.y - from.y);
        float top = std::max(0.0f, from.y);
        float bottom = std::min(static_cast<float>(_height), to.y);
        float x = from.x + (top - from.y) * slope;
        for (float row = std::floor(top); row < bottom; row += 1.0f)
        {
            float span = std::min(row + 1.0f, bottom) - std::max(row, top);
            float next = x + slope * span;
            addRowSpan(static_cast<std::size_t>(row) * _width, std::min(std::max(x, 0.0f), limit),
                std::min(std::max(next, 0.0f), limit), span * direction);
            x = next;
        }
    }

    // Число отрезков растёт как корень из отклонения контрольной точки (как в font-rs)
    // The segment count grows as the root of the control point deviation (as in font-rs)
    void CoverageAccumulator::addQuadratic(Vec2 from, Vec2 control, Vec2 to)
    {
        float dx = from.x - 2.0f * control.x + to.x;
        float dy = from.y - 2.0f * control.y + to.y;
        float deviation = dx * dx + dy * dy;
        int segments = 1 + static_cast<int>(std::floor(std::sqrt(std::sqrt(3.0f * deviation))));
        Vec2 previous = from;

        segments = std::min(segments, 64);
        for (int index = 1; index <= segments; ++index)
        {
            float t = static_cast<float>(index) / static_cast<float>(segments);
            float u = 1.0f - t;
            Vec2 point{u * u * from.x + 2.0f * u * t * control.x + t * t * to.x,
                u * u * from.y + 2.0f * u * t * control.y + t * t * to.y};
            addLine(previous, point);
            previous = point;
        }
    }

    // Нарастающая сумма по строке; модуль с насыщением — покрытие / A running sum along the row; the saturated absolute value is the coverage
    void CoverageAccumulator::resolve(std::vector<std::uint8_t>& coverage, bool evenOdd) const
    {
        coverage.assign(static_cast<std::size_t>(_width) * _height, 0);
        for (int row = 0; row < _height; ++row)
        {
            float sum = 0.0f;
            for (int column = 0; column < _width; ++column)
            {
                std::size_t index = static_cast<std::size_t>(row) * _width + column;
                sum += _cells[index];
                float value = std::min(1.0f, std::fabs(sum));
                // Чётно-нечётно: обмотка 2 снова пуста — «пила» с периодом 2 / Even-odd: winding 2 is empty again — a "saw" of period 2
                if (evenOdd)
                    value = 1.0f - std::fabs(1.0f - std::fmod(std::fabs(sum), 2.0f));
                coverage[index] = static_cast<std::uint8_t>(std::lround(value * 255.0f));
            }
        }
    }
}
