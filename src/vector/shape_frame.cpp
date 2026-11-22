#include "gran/vector/internal/shape_frame.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gran::vector
{
    namespace
    {
        // Проверка до доступа к точкам / Validate before indexing the points
        bool validPath(const Path& path)
        {
            if (!path.verbs.empty() && path.verbs.front() != Verb::MOVE)
                return false;
            std::size_t remaining = path.points.size();
            for (Verb verb : path.verbs)
            {
                const auto index = static_cast<std::size_t>(verb);
                if (index >= std::size(POINT_COUNTS) || POINT_COUNTS[index] > remaining)
                    return false;
                remaining -= POINT_COUNTS[index];
            }
            return remaining == 0;
        }

        // Границы проверяются до приведения к int / Check bounds before converting to int
        bool frameAxis(float low, float high, int& origin, int& length)
        {
            const double first = std::floor(static_cast<double>(low)) - 1;
            const double last = std::ceil(static_cast<double>(high)) + 2;
            if (first < std::numeric_limits<int>::min() || last > std::numeric_limits<int>::max()
                || last - first > 2048)
                return false;
            origin = static_cast<int>(first);
            length = static_cast<int>(last - first);
            return true;
        }
    }

    // Точки всех контуров в пикселях растра и рамка растра с запасом в пиксель / The points of all contours in raster pixels and the raster frame with a pixel margin
    bool frameShape(const Shape& shape, const Affine2D& toPixels, std::vector<std::vector<Vec2>>& points,
        text::GlyphBitmap& bitmap)
    {
        Vec2 low{1e30f, 1e30f};
        Vec2 high{-1e30f, -1e30f};

        for (const Path& path : shape.paths)
        {
            if (!validPath(path)) return false;
            points.emplace_back();
            for (const Vec2& source : path.points)
            {
                Vec2 point = affineApply(toPixels, source);
                if (!std::isfinite(point.x) || !std::isfinite(point.y)) return false;
                low = Vec2{std::min(low.x, point.x), std::min(low.y, point.y)};
                high = Vec2{std::max(high.x, point.x), std::max(high.y, point.y)};
                points.back().push_back(point);
            }
        }
        if (high.x < low.x || !frameAxis(low.x, high.x, bitmap.left, bitmap.width)
            || !frameAxis(low.y, high.y, bitmap.top, bitmap.height)) return false;
        return true;
    }
}
