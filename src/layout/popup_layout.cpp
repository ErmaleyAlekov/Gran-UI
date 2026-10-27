#include "gran/layout/internal/layout_internal.hpp"

#include <cmath>

namespace gran::layout::internal
{
    namespace
    {
        // Зазор между якорем-рамкой и слоем; у якоря-точки (контекстное меню) зазора нет / The gap between a frame anchor and the layer; a point anchor (a context menu) has none
        constexpr float ANCHOR_GAP = 4.0f;

        // Сдвиг рамки внутрь окна (слой больше окна прижимается к левому верхнему углу) / Shifting the frame into the window (a layer larger than the window is pressed to the top-left corner)
        Rect clampToWindow(Rect frame, Size viewport) noexcept
        {
            frame.x = std::fmax(0.0f, std::fmin(frame.x, viewport.width - frame.width));
            frame.y = std::fmax(0.0f, std::fmin(frame.y, viewport.height - frame.height));
            return frame;
        }

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
            return clampToWindow(frame, viewport);
        }

        /*
        ** Подменю: справа от пункта, первый пункт подменю — вровень с ним (верх слоя выше на его
        ** верхний отступ и рамку); справа не помещается — слева от родительского меню (отражение).
        **
        ** A submenu: to the right of the item, the first submenu item is level with it (the layer top is higher
        ** by its top padding and border); if it does not fit on the right — to the left of the parent menu (mirroring).
        */
        Rect placeSide(const ui::Node& submenu, const ui::Node& item, Size size, Size viewport) noexcept
        {
            const ui::Node* menu = item.parent;
            Rect parent = menu->layout.frame;
            Rect frame{parent.x + parent.width, item.layout.frame.y, size.width, size.height};

            frame.y -= submenu.computed.padding.top + submenu.computed.borderWidth.top;
            if (frame.x + size.width > viewport.width && parent.x - size.width >= 0.0f)
                frame.x = parent.x - size.width;
            return clampToWindow(frame, viewport);
        }

        /*
        ** Место слоя по его роли: диалог — по центру окна, подменю — сбоку от пункта, список ComboBox —
        ** под полем и не уже его (как <select>), прочие — у якоря из showPopup.
        **
        ** The layer place by its role: a dialog — in the window center, a submenu — beside the item, a ComboBox list —
        ** under the field and not narrower than it (as <select>), the others — at the anchor from showPopup.
        */
        Rect placeLayer(ui::Node& layer, Size size, Size viewport) noexcept
        {
            ui::Node* owner = layer.parent;

            if (layer.kind == ui::ElementKind::DIALOG)
                return clampToWindow(Rect{std::round((viewport.width - size.width) * 0.5f),
                    std::round((viewport.height - size.height) * 0.5f), size.width, size.height}, viewport);
            if (owner != nullptr && owner->kind == ui::ElementKind::MENU_ITEM)
                return placeSide(layer, *owner, size, viewport);
            if (owner != nullptr && owner->kind == ui::ElementKind::COMBO_BOX)
                size.width = std::fmax(size.width, layer.content.popupAnchor.width);
            return placePopup(layer.content.popupAnchor, size, viewport);
        }
    }

    /*
    ** Слой меряется по окну целиком (ему не мешают рамки родителя) и размещается по своей роли.
    ** Вложенные слои (подменю, меню и списки внутри диалога) — после своего слоя: их место
    ** зависит от только что размещённых пунктов.
    **
    ** A layer is measured by the whole window (the parent frames do not limit it) and placed by its role.
    ** Nested layers (submenus, menus and lists inside a dialog) come after their layer: their place
    ** depends on the items just placed.
    */
    void layoutPopups(ui::Node& node, Size viewport, LayoutPass& pass)
    {
        for (const std::unique_ptr<ui::Node>& child : node.children)
        {
            if (!child->visible)
                continue;
            if (ui::isLayer(*child))
            {
                Size size = measureNode(*child, viewport, pass);
                arrangeNode(*child, placeLayer(*child, size, viewport), pass);
            }
            layoutPopups(*child, viewport, pass);
        }
    }
}
