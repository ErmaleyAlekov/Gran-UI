#include "gran/text/glyph_rasterizer.hpp"

#include <algorithm>
#include <cmath>

namespace gran::text
{
    namespace
    {
        // Отрезок, пересекающий несколько ячеек строки / A segment crossing several cells of a row
        struct CellSpan
        {
            std::size_t first;
            std::size_t last;
            float lowFraction;
            float highFraction;
            float scale;
        };

        // Отрезок внутри одной ячейки: площадь делится по средней точке между ней и соседней справа
        // A segment inside one cell: the area is split by the middle point between it and its right neighbour
        void addNarrow(std::vector<float>& cells, std::size_t index, float middle, float delta)
        {
            cells[index] += delta - delta * middle;
            cells[index + 1] += delta * middle;
        }

        // Широкий отрезок: трапеции на краях, равные доли в середине (алгоритм font-rs)
        // A wide segment: trapezoids at the edges, equal shares in the middle (the font-rs algorithm)
        void addWide(std::vector<float>& cells, const CellSpan& span, float delta)
        {
            float startArea = 0.5f * span.scale * (1.0f - span.lowFraction) * (1.0f - span.lowFraction);
            float endArea = 0.5f * span.scale * span.highFraction * span.highFraction;

            cells[span.first] += delta * startArea;
            if (span.last == span.first + 2)
                cells[span.first + 1] += delta * (1.0f - startArea - endArea);
            else
            {
                float second = span.scale * (1.5f - span.lowFraction);
                cells[span.first + 1] += delta * (second - startArea);
                for (std::size_t cell = span.first + 2; cell < span.last - 1; ++cell)
                    cells[cell] += delta * span.scale;
                float beforeLast = second + static_cast<float>(span.last - span.first - 3) * span.scale;
                cells[span.last - 1] += delta * (1.0f - beforeLast - endArea);
            }
            cells[span.last] += delta * endArea;
        }
    }

    /*
    ** Площадь, которую отрезок высотой |delta| от x до xNext отсекает справа в строке,
    ** раскладывается по ячейкам. Вызывающий гарантирует 0 ≤ x, xNext < ширина − 1,
    ** поэтому индексы не выходят из строки.
    **
    ** The area the segment of height |delta| from x to xNext cuts off to the right in the row
    ** is spread over the cells. The caller guarantees 0 ≤ x, xNext < width − 1,
    ** so the indices do not leave the row.
    */
    void CoverageAccumulator::addRowSpan(std::size_t rowStart, float x, float xNext, float delta)
    {
        float low = std::min(x, xNext);
        float high = std::max(x, xNext);
        float lowFloor = std::floor(low);
        float highCeil = std::ceil(high);
        CellSpan span{rowStart + static_cast<std::size_t>(lowFloor), rowStart + static_cast<std::size_t>(highCeil),
            low - lowFloor, high - highCeil + 1.0f, 0.0f};

        if (span.last <= span.first + 1)
        {
            addNarrow(_cells, span.first, 0.5f * (x + xNext) - lowFloor, delta);
            return;
        }
        span.scale = 1.0f / (high - low);
        addWide(_cells, span, delta);
    }
}
