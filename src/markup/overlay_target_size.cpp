#include "gran/markup/internal/overlay.hpp"
#include "gran/a11y/access_tree.hpp"
#include "gran/style/value_parsers.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gran::markup::internal
{
    namespace
    {
        // Квадраты длин строк обратной матрицы и их скалярное произведение.
        // Squared inverse row lengths and their dot product across ancestor states.
        struct Metric
        {
            double xx = 1, xy = 0, yy = 1;
        };
        struct Range
        {
            Metric low, high;
            bool valid = true;
        };

        Metric transformMetric(const Affine2D& inverse, const Metric& m)
        {
            const double a = inverse.a, b = inverse.b, c = inverse.c, d = inverse.d;
            return {a*a*m.xx + 2*a*c*m.xy + c*c*m.yy,
                    a*b*m.xx + (a*d + b*c)*m.xy + c*d*m.yy,
                    b*b*m.xx + 2*b*d*m.xy + d*d*m.yy};
        }

        // Для фиксированного преобразования каждая компонента линейна по метрике родителя.
        // For a fixed transform each component is linear in the parent metric.
        Range childRange(const Range& parent, const std::vector<Affine2D>& variants)
        {
            if (!parent.valid) return parent;
            const double inf = std::numeric_limits<double>::infinity();
            Range next{{inf, inf, inf}, {-inf, -inf, -inf}};
            for (const auto& transform : variants)
            {
                Affine2D inverse;
                if (!affineInvert(transform, inverse)) return {{}, {}, false};
                for (double xx : {parent.low.xx, parent.high.xx})
                    for (double xy : {parent.low.xy, parent.high.xy})
                        for (double yy : {parent.low.yy, parent.high.yy})
                        {
                            const auto m = transformMetric(inverse, {xx, xy, yy});
                            if (!std::isfinite(m.xx) || !std::isfinite(m.xy) || !std::isfinite(m.yy))
                                return {{}, {}, false};
                            next.low = {std::min(next.low.xx,m.xx), std::min(next.low.xy,m.xy), std::min(next.low.yy,m.yy)};
                            next.high = {std::max(next.high.xx,m.xx), std::max(next.high.xy,m.xy), std::max(next.high.yy,m.yy)};
                        }
            }
            return next;
        }

        struct Path
        {
            const Path* parent;
            const std::vector<Affine2D>& variants;
            Range range;
        };

        // Огибающая отсекает ветки; решение принимается только по достижимым состояниям.
        // Bounds prune branches; only reachable state combinations determine the result.
        double largestExtent(const Path* path, double x, double y, double best = 0)
        {
            if (!path) return std::max(best, x*x + y*y);
            const auto& range = path->range;
            const double cross = 2*x*y;
            const double upper = x*x*range.high.xx + y*y*range.high.yy
                + cross*(cross >= 0 ? range.high.xy : range.low.xy);
            if (upper <= best) return best;
            const double lower = x*x*range.low.xx + y*y*range.low.yy
                + cross*(cross >= 0 ? range.low.xy : range.high.xy);
            if (lower == upper) return upper;
            for (const auto& transform : path->variants)
            {
                Affine2D inverse;
                affineInvert(transform, inverse);
                best = largestExtent(path->parent, x*inverse.a + y*inverse.b, x*inverse.c + y*inverse.d, best);
            }
            return best;
        }

        // Расстояние между противоположными гранями равно размеру, делённому на длину строки обратной матрицы.
        // The distance between opposite edges equals the layout extent divided by the inverse row length.
        void enforceSize(ui::Node& node, float minimum, const Path& path)
        {
            if (!path.range.valid) denyOverlay(*node.domElement, "minTargetSize");
            const auto& style = node.computed;
            const float width = static_cast<float>(minimum * std::sqrt(largestExtent(&path, 1, 0)));
            const float height = static_cast<float>(minimum * std::sqrt(largestExtent(&path, 0, 1)));
            if (!std::isfinite(width) || !std::isfinite(height)
                || (style.width.unit == style::LengthUnit::PIXELS && style.width.value < width)
                || (style.height.unit == style::LengthUnit::PIXELS && style.height.value < height)
                || style.maxWidth < width || style.maxHeight < height)
                denyOverlay(*node.domElement, "minTargetSize");
            style::setDeclaration(node.declared.base, style::PropertyId::MIN_WIDTH, std::max(style.minWidth, width));
            style::setDeclaration(node.declared.base, style::PropertyId::MIN_HEIGHT, std::max(style.minHeight, height));
        }

        void visit(ui::Node& node, float minimum, const TargetTransforms& transforms, const Path* parent)
        {
            if (node.domElement)
                if (const auto* size = findAttribute(*node.domElement, "minTargetSize"))
                    node.minimumHitSize = minimum = style::parseNumber(*size);
            if (ui::isLayer(node)) parent = nullptr;
            const auto& variants = transforms.at(&node);
            const Path path{parent, variants, childRange(parent ? parent->range : Range{}, variants)};
            if (minimum > 0 && node.domElement && a11y::isFocusable(node)) enforceSize(node, minimum, path);
            for (auto& child : node.children) visit(*child, minimum, transforms, &path);
        }
    }

    void recordTargetTransform(const ui::Node& node, TargetTransforms& transforms)
    {
        auto matrix = ui::nodeTransform(node);
        matrix.tx = matrix.ty = 0;
        auto& variants = transforms[&node];
        const auto same = [&matrix](const Affine2D& m)
        {
            return m.a == matrix.a && m.b == matrix.b && m.c == matrix.c && m.d == matrix.d;
        };
        if (std::none_of(variants.begin(), variants.end(), same)) variants.push_back(matrix);
        for (const auto& child : node.children) recordTargetTransform(*child, transforms);
    }

    void checkTargetSizes(ui::Node& root, float minimum, const TargetTransforms& transforms)
    {
        root.minimumHitSize = minimum;
        visit(root, minimum, transforms, nullptr);
    }
}
