#include "gran/render/internal/scene_internal.hpp"

#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        // Размер растра (в ¼ px устройства) и пиксели устройства на логический пиксель
        // The raster size (in ¼ device px) and device pixels per logical pixel
        struct RunScale
        {
            std::uint16_t quarter;
            float device;
        };

        // Координаты текстуры участка атласа / The texture coordinates of an atlas area
        void applyAtlasUv(GpuInstance& instance, const text::AtlasEntry& entry, float atlasSize)
        {
            instance.extra[0] = entry.x / atlasSize;
            instance.extra[1] = entry.y / atlasSize;
            instance.extra[2] = (entry.x + entry.width) / atlasSize;
            instance.extra[3] = (entry.y + entry.height) / atlasSize;
        }

        /*
        ** Выровненный глиф: базовая линия — на целом пикселе, перо по x — с точностью ¼ px
        ** (четыре варианта растра), как подпиксельное позиционирование в браузерах.
        **
        ** An aligned glyph: the baseline is on a whole pixel, the pen x is precise to ¼ px
        ** (four raster variants), as subpixel positioning in browsers.
        */
        void placeAlignedGlyph(const text::PositionedGlyph& glyph, Vec2 pen, RunScale scale,
            GpuInstance& instance, SceneContext& context)
        {
            float column = std::floor(pen.x);
            int subpixel = static_cast<int>(std::lround((pen.x - column) * 4.0f));

            if (subpixel == 4)
            {
                subpixel = 0;
                column += 1.0f;
            }
            text::GlyphKey key{glyph.face, glyph.glyph, scale.quarter, static_cast<std::uint8_t>(subpixel)};
            const text::AtlasEntry* entry = context.resources.atlas.glyph(key);
            if (entry == nullptr)
                return;
            instance.rect[0] = column + entry->left;
            instance.rect[1] = std::round(pen.y) + entry->top;
            instance.rect[2] = entry->width;
            instance.rect[3] = entry->height;
            instance.transform[0] = 1.0f;
            instance.transform[3] = 1.0f;
            applyAtlasUv(instance, *entry, static_cast<float>(context.resources.atlas.size()));
            context.scene.instances.push_back(instance);
        }

        // Повёрнутый или масштабированный глиф: растр под итоговый масштаб, квадрат — в логических координатах
        // A rotated or scaled glyph: the raster for the final scale, the quad in logical coordinates
        void placeTransformedGlyph(const text::PositionedGlyph& glyph, Vec2 logical, RunScale scale,
            GpuInstance& instance, SceneContext& context)
        {
            const text::AtlasEntry* entry = context.resources.atlas.glyph(
                text::GlyphKey{glyph.face, glyph.glyph, scale.quarter, 0});

            if (entry == nullptr)
                return;
            instance.rect[0] = logical.x + entry->left / scale.device;
            instance.rect[1] = logical.y + entry->top / scale.device;
            instance.rect[2] = entry->width / scale.device;
            instance.rect[3] = entry->height / scale.device;
            applyAtlasUv(instance, *entry, static_cast<float>(context.resources.atlas.size()));
            context.scene.instances.push_back(instance);
        }
    }

    // Глифы одним цветом и кеглем; участок — фильтр по номеру в глифе / The glyphs in one color and size; a run is a filter by the number in the glyph
    void paintGlyphs(const text::TextLayout& layout, Vec2 origin, const GlyphPaint& paint, const PaintState& state,
        SceneContext& context)
    {
        const Affine2D& t = state.transform;
        RunScale scale{0, std::sqrt(std::fabs(t.a * t.d - t.b * t.c))};
        float unit = 1.0f;
        GpuInstance instance = placeInstance(state, Rect{0.0f, 0.0f, 1.0f, 1.0f}, context.scale, unit);

        scale.quarter = static_cast<std::uint16_t>(std::lround(paint.size * scale.device * 4.0f));
        instance.kind = KIND_GLYPH;
        instance.fill = colorPackPremultiplied(paint.color);
        for (const text::PositionedGlyph& glyph : layout.glyphs)
        {
            if (paint.run != ALL_RUNS && glyph.run != paint.run)
                continue;
            Vec2 logical{origin.x + glyph.x, origin.y + glyph.y};
            if (state.snapped)
                placeAlignedGlyph(glyph, affineApply(t, logical), scale, instance, context);
            else
                placeTransformedGlyph(glyph, logical, scale, instance, context);
        }
    }

    /*
    ** Текст листа: место раскладки — textOrigin (центровка, сдвиг длинного поля), подсказка поля —
    ** полупрозрачная. Порядок: подложки участков, подсветка выделения, глифы (участками — своим
    ** цветом и кеглем), каретка.
    **
    ** The leaf text: the layout place is textOrigin (centering, the long input shift), the input placeholder
    ** is semi-transparent. The order: the run backgrounds, the selection highlight, the glyphs (by runs — in their
    ** color and size), the caret.
    */
    void paintText(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const std::string& content = displayedText(node);
        Rect box = contentRect(node);
        const text::TextLayout& layout = context.resources.text.layoutFor(node, box.width);
        TextPlace place{node, layout, textOrigin(node, layout, box)};
        GlyphPaint paint{node.computed.color, layout.fontSize, ALL_RUNS};
        bool runs = ui::hasSpans(node);

        if (ui::isTextField(node) && node.content.text.empty())
            paint.color.a *= 0.5f;
        if (runs)
            paintRunBackgrounds(place, state, context);
        if (context.resources.selection == &node)
            paintSelection(place, state, context);
        if (runs)
            paintRunGlyphs(place, state, context);
        else if (!content.empty())
            paintGlyphs(layout, place.origin, paint, state, context);
        if (context.resources.caret == &node)
            paintTextCaret(place, state, context);
    }
}
