#include "gran/ui/node.hpp"
#include "gran/a11y/access_tree.hpp"

#include <algorithm>
#include <cmath>

namespace gran::ui
{
    // Профиль может переместить узел между областями с разными ограничениями.
    // A profile can move a node between regions with different target limits.
    float inheritedHitSize(const Node* node) noexcept
    {
        for (; node; node = node->parent)
            if (node->minimumHitSize >= 0) return node->minimumHitSize;
        return 0;
    }

    // Дополнительный квадрат следует за видимым центром, даже если кадр анимации вырожден.
    // The additional square follows the visual centre even at a singular animation frame.
    bool minimumHitPoint(const Node& node, const Affine2D& m, float minimum, Vec2 windowPoint, Vec2& local) noexcept
    {
        if (minimum <= 0 || !a11y::isFocusable(node)) return false;
        const auto& frame = node.layout.frame;
        if (frame.width <= 0 || frame.height <= 0) return false;
        const double determinant = std::abs(double(m.a)*m.d - double(m.b)*m.c);
        if (determinant > 0 && frame.width * determinant >= minimum * std::hypot(m.c, m.d)
            && frame.height * determinant >= minimum * std::hypot(m.a, m.b)) return false;
        const Vec2 centre = affineApply(m, {frame.x + frame.width/2, frame.y + frame.height/2});
        const Rect area{centre.x - minimum/2, centre.y - minimum/2, minimum, minimum};
        if (!rectContains(area, windowPoint)) return false;
        local = {frame.x + (windowPoint.x - area.x) * frame.width / minimum,
                 frame.y + (windowPoint.y - area.y) * frame.height / minimum};
        return true;
    }

    // Обработчики получают те же координаты, по которым диспетчер выбрал цель.
    // Handlers use the same coordinates that selected their pointer target.
    Vec2 pointerToNode(const Node& node, Vec2 point) noexcept
    {
        Affine2D matrix, inverse;
        for (auto* current = &node; current; current = current->parent)
        {
            matrix = affineMultiply(nodeTransform(*current), matrix);
            if (isLayer(*current)) break;
        }
        const bool invertible = affineInvert(matrix, inverse);
        Vec2 local = invertible ? affineApply(inverse, point) : Vec2{-1.0e9f, -1.0e9f};
        if (!invertible || !rectContains(node.layout.frame, local))
        {
            Vec2 extra;
            if (minimumHitPoint(node, matrix, inheritedHitSize(&node), point, extra))
            {
                if (!invertible) return extra;
                const auto& frame = node.layout.frame;
                local.x = std::clamp(local.x, frame.x, frame.x + frame.width);
                local.y = std::clamp(local.y, frame.y, frame.y + frame.height);
            }
        }
        return local;
    }
}
