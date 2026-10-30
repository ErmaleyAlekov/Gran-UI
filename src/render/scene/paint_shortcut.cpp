#include "gran/render/internal/scene_internal.hpp"

#include <cmath>
#include <limits>

namespace gran::render::internal
{
    // Приглушённость подписи сочетания (как у стрелки подменю) / The shortcut label muting (as for the submenu arrow)
    constexpr float SHORTCUT_OPACITY = 0.6f;

    // По вертикали — по центру, как подпись пункта (textOrigin) / Vertically centered, as the item label (textOrigin)
    void paintShortcut(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        if (node.content.shortcut.empty())
            return;
        ui::Node label = ui::shortcutLabel(node);
        Rect box = contentRect(node);
        const text::TextLayout& layout = context.resources.text.layoutFor(label, std::numeric_limits<float>::infinity());
        float lineHeight = node.computed.fontSize * node.computed.lineHeight;
        Vec2 origin{box.x + box.width - std::ceil(layout.width), box.y + (box.height - std::fmax(layout.height, lineHeight)) * 0.5f};
        GlyphPaint paint{colorWithOpacity(node.computed.color, SHORTCUT_OPACITY), layout.fontSize, ALL_RUNS};

        paintGlyphs(layout, origin, paint, state, context);
    }
}
