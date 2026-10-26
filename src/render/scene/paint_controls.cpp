#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        // Скруглённый прямоугольник элемента управления / A rounded rectangle of a control
        struct ControlBox
        {
            Rect rect;
            float radius;
            Color fill;
            Color stroke;
            float border;
        };

        // Прямоугольник (и тень под ним, если shadow > 0) / A rectangle (and a shadow under it if shadow > 0)
        void emitBox(const ControlBox& box, float shadow, const PaintState& state, SceneContext& context)
        {
            float unit = 1.0f;
            GpuInstance instance = placeInstance(state, box.rect, context.scale, unit);

            for (float& radius : instance.radii)
                radius = std::min(box.radius * unit, std::min(instance.rect[2], instance.rect[3]) * 0.5f);
            if (shadow > 0.0f)
            {
                GpuInstance below = instance;
                below.kind = KIND_SHADOW;
                below.param = shadow * unit;
                below.extra[1] = shadow * 0.5f * unit;
                below.fill = colorPackPremultiplied(Color{0.0f, 0.0f, 0.0f, 0.3f});
                context.scene.instances.push_back(below);
            }
            instance.kind = KIND_RECT;
            instance.fill = colorPackPremultiplied(box.fill);
            for (float& side : instance.border)
                side = box.border * unit;
            instance.stroke = colorPackPremultiplied(box.stroke);
            context.scene.instances.push_back(instance);
        }

        // Полоса: концы и толщина / A bar: the ends and the thickness
        struct Bar
        {
            Vec2 from;
            Vec2 to;
            float thickness;
        };

        // Полоса (ножка галочки): прямоугольник, повёрнутый вокруг середины / A bar (a check mark leg): a rectangle rotated around its middle
        void emitBar(const Bar& line, const Color& color, const PaintState& state, SceneContext& context)
        {
            float dx = line.to.x - line.from.x;
            float dy = line.to.y - line.from.y;
            float length = std::sqrt(dx * dx + dy * dy) + line.thickness;
            float angle = std::atan2(dy, dx);
            PaintState bar = state;
            Affine2D place{std::cos(angle), std::sin(angle), -std::sin(angle), std::cos(angle),
                (line.from.x + line.to.x) * 0.5f, (line.from.y + line.to.y) * 0.5f};

            bar.transform = affineMultiply(state.transform, place);
            bar.snapped = false;
            emitBox(ControlBox{Rect{-length * 0.5f, -line.thickness * 0.5f, length, line.thickness}, line.thickness * 0.5f,
                color, color, 0.0f}, 0.0f, bar, context);
        }

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
