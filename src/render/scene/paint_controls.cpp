#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        /*
        ** Флажок: квадрат высотой ~1.15 em по центру строки; отмеченный — заливка accentColor
        ** и белая галочка, неотмеченный — рамка цветом текста с прозрачностью; затем подпись.
        **
        ** A check box: a square of ~1.15 em centered on the line; a checked one — an accentColor fill
        ** and a white check mark, an unchecked one — a border in the text color with transparency; then the label.
        */
        void paintCheckBox(const ui::Node& node, const PaintState& state, SceneContext& context)
        {
            const style::ComputedStyle& style = node.computed;
            Rect box = contentRect(node);
            float size = std::round(style.fontSize * 1.15f);
            float gap = std::round(style.fontSize * 0.5f);
            Rect square{box.x, box.y + std::round((box.height - size) * 0.5f), size, size};

            if (node.content.checked)
            {
                emitBox(ControlBox{square, 4.0f, style.accentColor, style.accentColor, 0.0f}, 0.0f, state, context);
                Color mark{1.0f, 1.0f, 1.0f, 1.0f};
                Vec2 corner{square.x + size * 0.42f, square.y + size * 0.70f};
                emitBar(Bar{Vec2{square.x + size * 0.24f, square.y + size * 0.52f}, corner, size * 0.12f}, mark, state, context);
                emitBar(Bar{corner, Vec2{square.x + size * 0.77f, square.y + size * 0.30f}, size * 0.12f}, mark, state, context);
            }
            else
                emitBox(ControlBox{square, 4.0f, Color{}, colorWithOpacity(style.color, 0.45f), 1.5f}, 0.0f, state, context);
            if (node.content.text.empty())
                return;
            const text::TextLayout& label = context.resources.text.layoutFor(node, std::max(0.0f, box.width - size - gap));
            paintGlyphs(label, Vec2{box.x + size + gap, box.y + (box.height - label.height) * 0.5f},
                GlyphPaint{style.color, label.fontSize, ALL_RUNS}, state, context);
        }

        // Ползунок: дорожка, заполненная часть до бегунка, круглый бегунок с тенью / A slider: the track, the filled part up to the thumb, a round thumb with a shadow
        void paintSlider(const ui::Node& node, const PaintState& state, SceneContext& context)
        {
            const style::ComputedStyle& style = node.computed;
            const ui::NodeContent& content = node.content;
            Rect box = contentRect(node);
            float thumb = std::round(style.fontSize * 1.15f);
            float range = std::max(content.maximum - content.minimum, 1e-6f);
            float t = std::min(1.0f, std::max(0.0f, (content.value - content.minimum) / range));
            float middle = box.y + box.height * 0.5f;
            float center = box.x + thumb * 0.5f + t * std::max(0.0f, box.width - thumb);

            emitBox(ControlBox{Rect{box.x, middle - 2.0f, box.width, 4.0f}, 2.0f, colorWithOpacity(style.accentColor, 0.25f),
                Color{}, 0.0f}, 0.0f, state, context);
            emitBox(ControlBox{Rect{box.x, middle - 2.0f, center - box.x, 4.0f}, 2.0f, style.accentColor, Color{}, 0.0f},
                0.0f, state, context);
            emitBox(ControlBox{Rect{center - thumb * 0.5f, middle - thumb * 0.5f, thumb, thumb}, thumb * 0.5f,
                Color{1.0f, 1.0f, 1.0f, 1.0f}, colorWithOpacity(style.accentColor, 0.9f), 2.0f}, 3.0f, state, context);
        }
    }

    // Встроенный элемент по виду узла / The built-in control by the node kind
    void paintControl(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        if (node.kind == ui::ElementKind::CHECKBOX)
            paintCheckBox(node, state, context);
        else
            paintSlider(node, state, context);
    }
}
