#include "gran/render/internal/scene_internal.hpp"

#include "gran/markup/customization.hpp"

#include <algorithm>

namespace gran::render::internal
{
    // Рамка и угловые ручки рисуются поверх содержимого тем же преобразованием.
    // Paint the frame and corner handles above the content with the same transform.
    void paintEditFrame(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const Rect rect = node.layout.frame;
        const Color accent = node.computed.accentColor;
        if (context.resources.editWindow == &node)
        {
            emitBox({rect, 0, colorWithOpacity(accent, 0), accent, 3}, 0, state, context);
            if (context.resources.editDrop != &node) return;
        }
        if (context.resources.editDrop == &node)
        {
            Rect mark = rect;
            const auto edge = context.resources.dropEdge;
            if (edge == DropEdge::LEFT || edge == DropEdge::RIGHT) mark.width = std::min(3.0f, rect.width);
            if (edge == DropEdge::TOP || edge == DropEdge::BOTTOM) mark.height = std::min(3.0f, rect.height);
            if (edge == DropEdge::RIGHT) mark.x += rect.width - mark.width;
            if (edge == DropEdge::BOTTOM) mark.y += rect.height - mark.height;
            const auto fill = edge == DropEdge::INSIDE ? colorWithOpacity(accent, 0.18f) : accent;
            emitBox({mark, 0, fill, accent, edge == DropEdge::INSIDE ? 2.0f : 0.0f}, 0, state, context);
            return;
        }
        emitBox({rect, 0, colorWithOpacity(accent, 0.04f), accent, 2}, 0, state, context);
        const auto* target = node.kind == ui::ElementKind::TAB_HEADER ? ui::pageOf(const_cast<ui::Node&>(node)) : &node;
        if (!target || !target->domElement || !markup::permitsCustomization(*target->domElement, markup::Customization::STYLE)) return;
        const float grip = std::min({6.0f, rect.width, rect.height});
        for (const float x : {rect.x, rect.x + rect.width - grip})
            for (const float y : {rect.y, rect.y + rect.height - grip})
                emitBox({{x, y, grip, grip}, 0, accent, accent, 0}, 0, state, context);
    }
}
