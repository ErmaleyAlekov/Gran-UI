#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Узел отсекает потомков (overflow: hidden или прокрутка) / The node clips its descendants (overflow: hidden or a scroll)
        bool clipsChildren(const ui::Node& node) noexcept
        {
            return node.kind == ui::ElementKind::SCROLL
                || node.computed.overflow == static_cast<int>(style::Overflow::HIDDEN);
        }

        /*
        ** Точка приходит в координатах родителя и переводится обратным преобразованием узла
        ** в его координаты компоновки (поворот, масштаб, сдвиг CSS). Дети проверяются в обратном
        ** порядке (верхний нарисован последним). Вне отсекающего узла его потомков не видно;
        ** вне обычного — потомки ещё могут выступать за его край.
        **
        ** The point comes in the parent coordinates and is turned by the node inverse transform
        ** into its layout coordinates (the CSS rotation, scale, shift). The children are checked in reverse
        ** order (the top one is painted last). Outside a clipping node its descendants are not visible;
        ** outside a plain one the descendants may still stick out of its edge.
        */
        ui::Node* hitNode(ui::Node& node, Vec2 point, const ui::Node* layer)
        {
            Affine2D inverse;

            // Слой (Popup, Dialog) проверяется отдельно (hitLayer), до дерева / A layer (a Popup, a Dialog) is checked separately (hitLayer), before the tree
            if (ui::isLayer(node) && &node != layer)
                return nullptr;
            if (!node.visible || !affineInvert(ui::nodeTransform(node), inverse))
                return nullptr;
            point = affineApply(inverse, point);
            bool inside = rectContains(node.layout.frame, point);
            if (!inside && clipsChildren(node))
                return nullptr;
            for (std::size_t index = node.children.size(); index > 0; --index)
            {
                ui::Node* hit = hitNode(*node.children[index - 1], point, layer);
                if (hit != nullptr)
                    return hit;
            }
            if (inside)
                return &node;
            return nullptr;
        }
    }

    // Попадание от корня / A hit from the root
    ui::Node* hitTest(ui::Node& root, Vec2 point)
    {
        return hitNode(root, point, nullptr);
    }

    // Слой живёт в координатах окна: преобразования предков не применяются / A layer lives in window coordinates: the ancestor transforms are not applied
    ui::Node* hitLayer(ui::Node& popup, Vec2 point)
    {
        return hitNode(popup, point, &popup);
    }
}
