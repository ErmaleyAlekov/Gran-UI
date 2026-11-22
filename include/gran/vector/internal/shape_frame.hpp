#ifndef GRAN_VECTOR_SHAPE_FRAME_HPP
#define GRAN_VECTOR_SHAPE_FRAME_HPP

#include "gran/vector/shape.hpp"

namespace gran::vector
{
    // Размер шага в массиве точек / Number of points consumed by each path verb
    inline constexpr std::size_t POINT_COUNTS[] = {1, 1, 2, 3, 0};

    bool frameShape(const Shape& shape, const Affine2D& toPixels, std::vector<std::vector<Vec2>>& points,
        text::GlyphBitmap& bitmap);
}
#endif
