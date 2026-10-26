#include "gran/render/internal/scene_internal.hpp"

#include <algorithm>

namespace gran::render::internal
{
    namespace
    {
        // Подложка участка — только сплошной цвет (градиент по кускам строк выглядел бы рваным)
        // A run background is a plain color only (a gradient over line pieces would look torn)
        bool runBackground(const ui::Node& span, Color& color) noexcept
        {
            if (span.computed.background.kind != style::PaintKind::COLOR || span.computed.background.color.a <= 0.0f)
                return false;
            color = span.computed.background.color;
            return true;
        }

        /*
        ** Линия оформления участка: подчёркивание — чуть ниже базовой линии, зачёркивание — на
        ** высоте середины строчных букв; толщина — 1/14 кегля, но не меньше пикселя.
        **
        ** The run decoration line: an underline is slightly below the baseline, a line-through is at
        ** the middle of the lowercase letters; the thickness is 1/14 of the font size, but not less than a pixel.
        */
        void paintDecoration(const TextPlace& place, const ui::Node& span, RangeFill fill, const PaintState& state,
            SceneContext& context)
        {
            float size = span.computed.fontSize;
            float baseline = place.layout.lines.front().baseline;

            if (span.computed.textDecoration == static_cast<int>(style::TextDecoration::UNDERLINE))
                fill.top = baseline + size * 0.12f;
            else
                fill.top = baseline - size * 0.28f;
            fill.height = std::max(1.0f, size / 14.0f);
            fill.color = span.computed.color;
            paintRange(place, fill, state, context);
        }
    }

    // Подложки участков (под выделением и глифами), со скруглением в 3 пикселя / The run backgrounds (under the selection and the glyphs), rounded by 3 pixels
    void paintRunBackgrounds(const TextPlace& place, const PaintState& state, SceneContext& context)
    {
        RangeFill fill;

        fill.radius = 3.0f;
        for (const std::unique_ptr<ui::Node>& span : place.node.children)
        {
            fill.to = fill.from + span->content.text.size();
            if (runBackground(*span, fill.color))
                paintRange(place, fill, state, context);
            fill.from = fill.to;
        }
    }

    /*
    ** Глифы по участкам: у каждого участка свой цвет и кегль (номер участка записан в глифе),
    ** затем линия оформления (textDecoration) его цветом.
    **
    ** The glyphs by runs: each run has its own color and font size (the run number is stored in the glyph),
    ** then the decoration line (textDecoration) in its color.
    */
    void paintRunGlyphs(const TextPlace& place, const PaintState& state, SceneContext& context)
    {
        RangeFill fill;
        GlyphPaint paint;

        paint.run = 0;
        for (const std::unique_ptr<ui::Node>& span : place.node.children)
        {
            fill.to = fill.from + span->content.text.size();
            paint.color = span->computed.color;
            paint.size = span->computed.fontSize;
            paintGlyphs(place.layout, place.origin, paint, state, context);
            if (span->computed.textDecoration != 0 && !place.layout.lines.empty())
                paintDecoration(place, *span, fill, state, context);
            fill.from = fill.to;
            ++paint.run;
        }
    }
}
