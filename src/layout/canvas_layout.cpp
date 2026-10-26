#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Незаданная координата (NaN) считается нулём / An unset coordinate (NaN) counts as zero
        float orZero(float value) noexcept
        {
            if (std::isnan(value))
                return 0.0f;
            return value;
        }

        /*
        ** Положение и размер по одной оси канваса: начало (left/top), конец (right/bottom),
        ** размер по измерению; при обоих краях и авто-размере — растяжение между ними.
        **
        ** The position and size along one canvas axis: the start (left/top), the end (right/bottom),
        ** the size by measurement; with both edges and an auto size — stretching between them.
        */
        void placeAxis(float start, float end, bool fixedSize, float room, Vec2& placement) noexcept
        {
            // placement.x — позиция, placement.y — размер / placement.x is the position, placement.y is the size
            placement.x = 0.0f;
            if (!std::isnan(start) && !std::isnan(end) && !fixedSize)
                placement.y = std::max(0.0f, room - start - end);
            if (!std::isnan(start))
                placement.x = start;
            else if (!std::isnan(end))
                placement.x = room - end - placement.y;
        }
    }

    // Размер канваса по содержимому — наибольший правый нижний край детей
    // The canvas content size is the largest bottom right edge of the children
    Size measureCanvas(ui::Node& canvas, Size contentAvailable, LayoutPass& pass)
    {
        Size extent{0.0f, 0.0f};

        for (const std::unique_ptr<ui::Node>& child : canvas.children)
        {
            if (!ui::inFlow(*child))
                continue;
            const style::ComputedStyle& style = child->computed;
            Size size = measureNode(*child, shrinkAvailable(contentAvailable, style.margin), pass);
            float left = orZero(style.left);
            float top = orZero(style.top);
            extent.width = std::max(extent.width, left + style.margin.left + size.width + style.margin.right);
            extent.height = std::max(extent.height, top + style.margin.top + size.height + style.margin.bottom);
        }
        return extent;
    }

    // Каждый ребёнок по своим краям, затем со сдвигом на внешние отступы / Every child by its edges, then shifted by its margins
    void arrangeCanvas(ui::Node& canvas, Rect content, LayoutPass& pass)
    {
        for (const std::unique_ptr<ui::Node>& child : canvas.children)
        {
            if (!ui::inFlow(*child))
                continue;
            const style::ComputedStyle& style = child->computed;
            Rect room = rectDeflate(Rect{0.0f, 0.0f, content.width, content.height}, style.margin);
            Size size = measureNode(*child, Size{room.width, room.height}, pass);
            Vec2 across{0.0f, size.width};
            Vec2 down{0.0f, size.height};
            placeAxis(style.left, style.right, hasFixedSize(*child, true), room.width, across);
            placeAxis(style.top, style.bottom, hasFixedSize(*child, false), room.height, down);
            arrangeNode(*child, Rect{content.x + style.margin.left + across.x, content.y + style.margin.top + down.x,
                across.y, down.y}, pass);
        }
    }
}
