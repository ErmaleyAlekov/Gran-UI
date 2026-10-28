#include "gran/vector/shape.hpp"

#include <algorithm>
#include <cmath>

namespace gran::vector
{
    namespace
    {
        // Предел стороны растра иконки (как у глифов) / The icon raster side limit (as for glyphs)
        constexpr int MAX_RASTER = 2048;

        // Точек у шага контура в порядке Verb / The points of a contour step in the Verb order
        const std::size_t POINT_COUNTS[] = {1, 1, 2, 3, 0};

        /*
        ** Кубическая кривая отрезками. Число шагов — по вторым разностям контрольных точек:
        ** отклонение ломаной не больше 0,05 px (оценка ¾·d/n²), но не больше 64 шагов.
        **
        ** A cubic curve as segments. The step count is by the second differences of the control points:
        ** the polyline deviation is at most 0.05 px (the estimate ¾·d/n²), but at most 64 steps.
        */
        void addCubic(text::CoverageAccumulator& accumulator, const Vec2* p)
        {
            float ax = p[0].x - 2.0f * p[1].x + p[2].x;
            float ay = p[0].y - 2.0f * p[1].y + p[2].y;
            float bx = p[1].x - 2.0f * p[2].x + p[3].x;
            float by = p[1].y - 2.0f * p[2].y + p[3].y;
            float spread = std::sqrt(std::max(ax * ax + ay * ay, bx * bx + by * by));
            int steps = std::min(64, std::max(1, static_cast<int>(std::ceil(std::sqrt(spread * 0.75f / 0.05f)))));
            Vec2 previous = p[0];

            for (int step = 1; step <= steps; ++step)
            {
                float t = static_cast<float>(step) / static_cast<float>(steps);
                float u = 1.0f - t;
                Vec2 point{u * u * u * p[0].x + 3.0f * u * u * t * p[1].x + 3.0f * u * t * t * p[2].x + t * t * t * p[3].x,
                    u * u * u * p[0].y + 3.0f * u * u * t * p[1].y + 3.0f * u * t * t * p[2].y + t * t * t * p[3].y};
                accumulator.addLine(previous, point);
                previous = point;
            }
        }

        /*
        ** Контур в накопитель (точки уже в пикселях растра). Заливка замыкает каждый подконтур:
        ** перед новым MOVE и в конце — отрезок к началу подконтура.
        **
        ** A contour into the accumulator (the points are already in raster pixels). A fill closes every subpath:
        ** before a new MOVE and at the end — a segment to the subpath start.
        */
        void feedPath(text::CoverageAccumulator& accumulator, const Path& path, const std::vector<Vec2>& points)
        {
            std::size_t next = 0;
            Vec2 current;
            Vec2 start;

            for (Verb verb : path.verbs)
            {
                if (verb == Verb::MOVE || verb == Verb::CLOSE)
                    accumulator.addLine(current, start);
                if (verb == Verb::MOVE)
                    start = points[next];
                if (verb == Verb::LINE)
                    accumulator.addLine(current, points[next]);
                if (verb == Verb::QUAD)
                    accumulator.addQuadratic(current, points[next], points[next + 1]);
                if (verb == Verb::CUBIC)
                {
                    Vec2 cubic[4] = {current, points[next], points[next + 1], points[next + 2]};
                    addCubic(accumulator, cubic);
                }
                next += POINT_COUNTS[static_cast<std::size_t>(verb)];
                current = start;
                if (verb != Verb::CLOSE)
                    current = points[next - 1];
            }
            accumulator.addLine(current, start);
        }

        // Точки всех контуров в пикселях растра и рамка растра с запасом в пиксель / The points of all contours in raster pixels and the raster frame with a pixel margin
        bool frameShape(const Shape& shape, const Affine2D& toPixels, std::vector<std::vector<Vec2>>& points,
            text::GlyphBitmap& bitmap)
        {
            Vec2 low{1e30f, 1e30f};
            Vec2 high{-1e30f, -1e30f};

            for (const Path& path : shape.paths)
            {
                points.emplace_back();
                for (const Vec2& source : path.points)
                {
                    Vec2 point = affineApply(toPixels, source);
                    low = Vec2{std::min(low.x, point.x), std::min(low.y, point.y)};
                    high = Vec2{std::max(high.x, point.x), std::max(high.y, point.y)};
                    points.back().push_back(point);
                }
            }
            bitmap.left = static_cast<int>(std::floor(low.x)) - 1;
            bitmap.top = static_cast<int>(std::floor(low.y)) - 1;
            bitmap.width = static_cast<int>(std::ceil(high.x)) + 2 - bitmap.left;
            bitmap.height = static_cast<int>(std::ceil(high.y)) + 2 - bitmap.top;
            return high.x >= low.x && bitmap.width <= MAX_RASTER && bitmap.height <= MAX_RASTER;
        }

        // Объединение покрытий: a + b − a·b (так же, как источник поверх источника) / Uniting the coverages: a + b − a·b (as a source over a source)
        void unite(std::vector<std::uint8_t>& total, const std::vector<std::uint8_t>& layer)
        {
            for (std::size_t index = 0; index < total.size(); ++index)
                total[index] = static_cast<std::uint8_t>(total[index] + layer[index] - total[index] * layer[index] / 255);
        }
    }

    /*
    ** viewBox → пиксели: сдвиг на начало viewBox и растяжение на width × height; точки сдвигаются
    ** в рамку растра. Каждый контур — свой накопитель (у каждого своё правило заливки), затем объединение.
    **
    ** viewBox → pixels: a shift by the viewBox start and stretching over width × height; the points are shifted
    ** into the raster frame. Every contour has its own accumulator (each has its own fill rule), then the union.
    */
    bool rasterizeShape(const Shape& shape, float width, float height, text::GlyphBitmap& bitmap)
    {
        Affine2D toPixels;
        std::vector<std::vector<Vec2>> points;

        bitmap = text::GlyphBitmap{};
        toPixels.a = width / shape.viewBox.width;
        toPixels.d = height / shape.viewBox.height;
        toPixels.tx = -shape.viewBox.x * toPixels.a;
        toPixels.ty = -shape.viewBox.y * toPixels.d;
        if (shape.paths.empty() || shape.viewBox.width <= 0.0f || shape.viewBox.height <= 0.0f
            || !frameShape(shape, toPixels, points, bitmap))
            return false;
        bitmap.coverage.assign(static_cast<std::size_t>(bitmap.width) * bitmap.height, 0);
        for (std::size_t index = 0; index < shape.paths.size(); ++index)
        {
            text::CoverageAccumulator accumulator(bitmap.width, bitmap.height);
            std::vector<std::uint8_t> layer;

            for (Vec2& point : points[index])
                point = Vec2{point.x - static_cast<float>(bitmap.left), point.y - static_cast<float>(bitmap.top)};
            feedPath(accumulator, shape.paths[index], points[index]);
            accumulator.resolve(layer, shape.paths[index].evenOdd);
            unite(bitmap.coverage, layer);
        }
        return true;
    }
}
