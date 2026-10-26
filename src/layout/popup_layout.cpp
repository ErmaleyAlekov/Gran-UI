#include "gran/layout/internal/layout_internal.hpp"

#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Зазор между якорем-рамкой и слоем; у якоря-точки (контекстное меню) зазора нет / The gap between a frame anchor and the layer; a point anchor (a context menu) has none
        constexpr float ANCHOR_GAP = 4.0f;

        /*
        ** Место слоя: под якорем от его левого края; не помещается снизу, а сверху есть место —
        ** над якорем (как выпадающие списки в браузерах); по горизонтали сдвигается внутрь окна.
        **
        ** The layer place: under the anchor from its left edge; if it does not fit below and there is room
        ** above — over the anchor (as browser drop-down lists); horizontally it is shifted into the window.
        */
        Rect placePopup(const Rect& anchor, Size size, Size viewport) noexcept
        {
            float gap = 0.0f;
            Rect frame{anchor.x, 0.0f, size.width, size.height};

            if (anchor.height > 0.0f)
                gap = ANCHOR_GAP;
            frame.y = anchor.y + anchor.height + gap;
            if (frame.y + size.height > viewport.height && anchor.y - gap - size.height >= 0.0f)
                frame.y = anchor.y - gap - size.height;
            if (anchor.height <= 0.0f && frame.x + size.width > viewport.width && anchor.x - size.width >= 0.0f)
                frame.x = anchor.x - size.width;
            frame.x = std::fmax(0.0f, std::fmin(frame.x, viewport.width - size.width));
            frame.y = std::fmax(0.0f, std::fmin(frame.y, viewport.height - size.height));
            return frame;
        }
    }

    /*
    ** Слой меряется по окну целиком (ему не мешают рамки родителя) и размещается у якоря.
    ** Вложенные слои внутри слоя не ищутся: меню открывается рядом, а не внутри другого меню.
    **
    ** A layer is measured by the whole window (the parent frames do not limit it) and placed at the anchor.
    ** Nested layers inside a layer are not searched: a menu opens beside, not inside another menu.
    */
    void layoutPopups(ui::Node& node, Size viewport, LayoutPass& pass)
    {
        for (const std::unique_ptr<ui::Node>& child : node.children)
        {
            if (!child->visible)
                continue;
            if (child->kind != ui::ElementKind::POPUP)
            {
                layoutPopups(*child, viewport, pass);
                continue;
            }
            Size size = measureNode(*child, viewport, pass);
            arrangeNode(*child, placePopup(child->content.popupAnchor, size, viewport), pass);
        }
    }
}
