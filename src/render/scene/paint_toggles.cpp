#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    // Подпись справа от значка, по центру строки / The label to the right of the mark, centered on the line
    void paintToggleLabel(const ui::Node& node, float markWidth, const PaintState& state, SceneContext& context)
    {
        const style::ComputedStyle& style = node.computed;
        Rect box = contentRect(node);
        float gap = std::round(style.fontSize * 0.5f);

        if (node.content.text.empty())
            return;
        const text::TextLayout& label = context.resources.text.layoutFor(node, std::max(0.0f, box.width - markWidth - gap));
        paintGlyphs(label, Vec2{box.x + markWidth + gap, box.y + (box.height - label.height) * 0.5f},
            GlyphPaint{style.color, label.fontSize, ALL_RUNS}, state, context);
    }

    /*
    ** Радиокнопка: круг высотой ~1.15 em по центру строки; отмеченная — заливка accentColor
    ** и белая точка в центре (как флажок с галочкой), неотмеченная — кольцо цветом текста с прозрачностью.
    **
    ** A radio button: a circle of ~1.15 em centered on the line; a checked one — an accentColor fill
    ** and a white dot in the center (as a check box with a check mark), an unchecked one — a ring in the text color with transparency.
    */
    void paintRadioButton(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const style::ComputedStyle& style = node.computed;
        Rect box = contentRect(node);
        float size = std::round(style.fontSize * 1.15f);
        Rect circle{box.x, box.y + std::round((box.height - size) * 0.5f), size, size};

        if (node.content.checked)
        {
            float dot = std::round(size * 0.4f);
            float inset = (size - dot) * 0.5f;
            emitBox(ControlBox{circle, size * 0.5f, style.accentColor, style.accentColor, 0.0f}, 0.0f, state, context);
            emitBox(ControlBox{Rect{circle.x + inset, circle.y + inset, dot, dot}, dot * 0.5f, Color{1.0f, 1.0f, 1.0f, 1.0f},
                Color{}, 0.0f}, 0.0f, state, context);
        }
        else
            emitBox(ControlBox{circle, size * 0.5f, Color{}, colorWithOpacity(style.color, 0.45f), 1.5f}, 0.0f, state, context);
        paintToggleLabel(node, size, state, context);
    }

    /*
    ** Переключатель: дорожка-«таблетка» 1.8 × высоту и белый круглый бегунок с тенью; включённый —
    ** дорожка accentColor и бегунок справа, выключенный — дорожка цветом текста с прозрачностью, бегунок слева.
    **
    ** A switch: a "pill" track of 1.8 × the height and a white round knob with a shadow; an on one —
    ** an accentColor track and the knob on the right, an off one — a track in the text color with transparency, the knob on the left.
    */
    void paintSwitch(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const style::ComputedStyle& style = node.computed;
        Rect box = contentRect(node);
        float height = std::round(style.fontSize * 1.15f);
        float width = std::round(height * 1.8f);
        float knob = height - 4.0f;
        Rect track{box.x, box.y + std::round((box.height - height) * 0.5f), width, height};
        Color fill = colorWithOpacity(style.color, 0.25f);
        float knobX = track.x + 2.0f;

        if (node.content.checked)
        {
            fill = style.accentColor;
            knobX = track.x + width - 2.0f - knob;
        }
        emitBox(ControlBox{track, height * 0.5f, fill, Color{}, 0.0f}, 0.0f, state, context);
        emitBox(ControlBox{Rect{knobX, track.y + 2.0f, knob, knob}, knob * 0.5f, Color{1.0f, 1.0f, 1.0f, 1.0f}, Color{}, 0.0f},
            2.0f, state, context);
        paintToggleLabel(node, width, state, context);
    }
}
