#include "gran/ui/node.hpp"

#include <cmath>

namespace gran::ui
{
    namespace
    {
        // Матрица одного шага CSS-преобразования (ось y вниз, поворот по часовой) / The matrix of one CSS transform step (the y axis down, rotation clockwise)
        Affine2D stepMatrix(const style::TransformStep& step) noexcept
        {
            Affine2D matrix;

            if (step.kind == style::TransformKind::TRANSLATE)
            {
                matrix.tx = step.x;
                matrix.ty = step.y;
            }
            else if (step.kind == style::TransformKind::SCALE)
            {
                matrix.a = step.x;
                matrix.d = step.y;
            }
            else if (step.kind == style::TransformKind::ROTATE)
            {
                matrix = Affine2D{std::cos(step.x), std::sin(step.x), -std::sin(step.x), std::cos(step.x), 0.0f, 0.0f};
            }
            else
            {
                matrix.b = std::tan(step.y);
                matrix.c = std::tan(step.x);
            }
            return matrix;
        }
    }

    /*
    ** Преобразование вращается вокруг transformOrigin блока: T(опора) · шаги слева направо · T(−опора).
    ** Линейная часть (a, b, c, d) совпадает с произведением шагов — по ней отрисовка решает,
    ** можно ли выравнивать по пикселям.
    **
    ** The transform rotates around the box transformOrigin: T(pivot) · the steps left to right · T(−pivot).
    ** The linear part (a, b, c, d) equals the product of the steps — rendering decides by it
    ** whether pixel alignment is possible.
    */
    Affine2D nodeTransform(const Node& node) noexcept
    {
        const style::ComputedStyle& style = node.computed;
        const Rect& frame = node.layout.frame;
        Affine2D local;

        if (style.transform.empty())
            return local;
        for (const style::TransformStep& step : style.transform)
            local = affineMultiply(local, stepMatrix(step));
        Vec2 pivot{frame.x + style.transformOrigin.x * frame.width, frame.y + style.transformOrigin.y * frame.height};
        Affine2D toPivot{1.0f, 0.0f, 0.0f, 1.0f, pivot.x, pivot.y};
        Affine2D fromPivot{1.0f, 0.0f, 0.0f, 1.0f, -pivot.x, -pivot.y};
        return affineMultiply(toPivot, affineMultiply(local, fromPivot));
    }

    // Произведение от корня к узлу: M(корень) · … · M(узел) / The product from the root to the node: M(root) · … · M(node)
    Affine2D nodeToWindow(const Node& node) noexcept
    {
        Affine2D total;

        for (const Node* current = &node; current != nullptr; current = current->parent)
            total = affineMultiply(nodeTransform(*current), total);
        return total;
    }

    // Обратное к nodeToWindow; вырожденное преобразование (масштаб 0) — точка заведомо вне узла
    // The inverse of nodeToWindow; a degenerate transform (scale 0) — a point surely outside the node
    Vec2 windowToNode(const Node& node, Vec2 point) noexcept
    {
        Affine2D inverse;

        if (!affineInvert(nodeToWindow(node), inverse))
            return Vec2{-1.0e9f, -1.0e9f};
        return affineApply(inverse, point);
    }
}
