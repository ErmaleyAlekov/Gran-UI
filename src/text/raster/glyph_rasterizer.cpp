#include "gran/text/glyph_rasterizer.hpp"

#include <algorithm>
#include <cmath>

namespace gran::text
{
    namespace
    {
        // Предел размера растра глифа (защита от огромного масштаба) / The glyph raster size limit (a guard against a huge scale)
        constexpr int MAX_RASTER = 2048;

        // Середина двух точек / The middle of two points
        Vec2 middle(Vec2 first, Vec2 second) noexcept
        {
            return Vec2{(first.x + second.x) * 0.5f, (first.y + second.y) * 0.5f};
        }

        // Состояние пера при обходе контура / The pen state while walking a contour
        struct PenState
        {
            Vec2 current;
            Vec2 control;
            bool hasControl = false;
        };

        /*
        ** Очередная точка контура TrueType: точка вне кривой — контрольная; две контрольные
        ** подряд подразумевают точку на кривой посередине.
        **
        ** The next TrueType contour point: an off-curve point is a control point; two control points
        ** in a row imply an on-curve point in the middle.
        */
        void feedPoint(PenState& pen, Vec2 point, bool onCurve, CoverageAccumulator& accumulator)
        {
            if (!onCurve && pen.hasControl)
            {
                Vec2 implied = middle(pen.control, point);
                accumulator.addQuadratic(pen.current, pen.control, implied);
                pen.current = implied;
            }
            if (!onCurve)
            {
                pen.control = point;
                pen.hasControl = true;
                return;
            }
            if (pen.hasControl)
                accumulator.addQuadratic(pen.current, pen.control, point);
            else
                accumulator.addLine(pen.current, point);
            pen.current = point;
            pen.hasControl = false;
        }

        // Контур от первой точки на кривой (или от середины первых двух, если таких нет) и замыкание
        // A contour from the first on-curve point (or from the middle of the first two if there is none) and the closing
        void walkContour(const std::vector<Vec2>& points, const Outline& outline, std::size_t first, std::size_t last,
            CoverageAccumulator& accumulator)
        {
            std::size_t count = last - first + 1;
            std::size_t origin = 0;
            PenState pen;

            while (origin < count && outline.onCurve[first + origin] == 0)
                ++origin;
            Vec2 start = middle(points[first], points[first + 1]);
            if (origin < count)
                start = points[first + origin];
            pen.current = start;
            for (std::size_t step = 1; step <= count; ++step)
            {
                std::size_t index = first + (origin + step) % count;
                feedPoint(pen, points[index], outline.onCurve[index] != 0, accumulator);
            }
            feedPoint(pen, start, true, accumulator);
        }

        // Точки в пикселях и рамка растра с запасом в пиксель / The points in pixels and the raster frame with a pixel margin
        bool transformPoints(const Outline& outline, const Affine2D& toPixels, std::vector<Vec2>& points, GlyphBitmap& bitmap)
        {
            Vec2 low{1e30f, 1e30f};
            Vec2 high{-1e30f, -1e30f};

            for (const Vec2& source : outline.points)
            {
                Vec2 point = affineApply(toPixels, source);
                low = Vec2{std::min(low.x, point.x), std::min(low.y, point.y)};
                high = Vec2{std::max(high.x, point.x), std::max(high.y, point.y)};
                points.push_back(point);
            }
            bitmap.left = static_cast<int>(std::floor(low.x)) - 1;
            bitmap.top = static_cast<int>(std::floor(low.y)) - 1;
            bitmap.width = static_cast<int>(std::ceil(high.x)) + 2 - bitmap.left;
            bitmap.height = static_cast<int>(std::ceil(high.y)) + 1 - bitmap.top;
            for (Vec2& point : points)
                point = Vec2{point.x - static_cast<float>(bitmap.left), point.y - static_cast<float>(bitmap.top)};
            return bitmap.width > 0 && bitmap.height > 0 && bitmap.width <= MAX_RASTER && bitmap.height <= MAX_RASTER;
        }
    }

    // Контуры по очереди в один накопитель / The contours one by one into one accumulator
    bool rasterizeOutline(const Outline& outline, const Affine2D& toPixels, GlyphBitmap& bitmap)
    {
        std::vector<Vec2> points;
        std::size_t first = 0;

        bitmap = GlyphBitmap{};
        if (outline.points.empty() || outline.onCurve.size() != outline.points.size()
            || !transformPoints(outline, toPixels, points, bitmap))
            return false;
        CoverageAccumulator accumulator(bitmap.width, bitmap.height);
        for (std::uint16_t end : outline.contourEnds)
        {
            if (end >= points.size() || end < first)
                break;
            if (end > first)
                walkContour(points, outline, first, end, accumulator);
            first = static_cast<std::size_t>(end) + 1;
        }
        accumulator.resolve(bitmap.coverage);
        return true;
    }
}
