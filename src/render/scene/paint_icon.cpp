#include "gran/render/internal/scene_internal.hpp"
#include "gran/render/content_sizer.hpp"

#include <algorithm>
#include <cmath>

namespace gran::render::internal
{
    namespace
    {
        // Растр фигуры по требованию атласа (паттерн Command: атлас вызывает только при промахе) / The shape raster on the atlas demand (the Command pattern: the atlas calls it only on a miss)
        struct ShapeRaster final : text::RasterSource
        {
            ShapeRaster(const vector::Shape& source, float rasterWidth, float rasterHeight) noexcept
                : shape(source), width(rasterWidth), height(rasterHeight)
            {
            }

            bool rasterize(text::GlyphBitmap& bitmap) override
            {
                return vector::rasterizeShape(shape, width, height, bitmap);
            }

            const vector::Shape& shape;
            float width;
            float height;
        };

        // Размер в ¼ пикселя устройства для ключа атласа (растры больше предела всё равно не строятся) / The size in ¼ device pixel for the atlas key (rasters above the limit are not built anyway)
        std::uint16_t quarterPixels(float pixels) noexcept
        {
            return static_cast<std::uint16_t>(std::min(65535.0f, std::max(0.0f, std::round(pixels * 4.0f))));
        }
    }

    namespace
    {
        /*
        ** Место растра: выровненный блок — в пикселях устройства от округлённого угла (как глифы),
        ** иначе — в логических координатах под преобразованием ветви.
        **
        ** The raster place: an aligned box — in device pixels from the rounded corner (as glyphs),
        ** otherwise — in logical coordinates under the branch transform.
        */
        void placeRaster(GpuInstance& instance, const text::AtlasEntry& entry, const Rect& box, const PaintState& state,
            float device)
        {
            Vec2 origin = affineApply(state.transform, Vec2{box.x, box.y});

            instance.rect[0] = box.x + entry.left / device;
            instance.rect[1] = box.y + entry.top / device;
            instance.rect[2] = entry.width / device;
            instance.rect[3] = entry.height / device;
            if (!state.snapped)
                return;
            instance.rect[0] = std::round(origin.x) + entry.left;
            instance.rect[1] = std::round(origin.y) + entry.top;
            instance.rect[2] = entry.width;
            instance.rect[3] = entry.height;
            instance.transform[0] = 1.0f;
            instance.transform[3] = 1.0f;
        }
    }

    /*
    ** Иконка растягивается на прямоугольник содержимого (как SVG без preserveAspectRatio — у квадратных
    ** иконок разницы нет). Растр строится под итоговый размер в пикселях устройства (DPI, масштаб, поворот),
    ** поэтому иконка резкая при любом увеличении. Выровненный блок встаёт на целые пиксели — края чёткие.
    **
    ** The icon is stretched over the content rectangle (as SVG without preserveAspectRatio — square icons
    ** see no difference). The raster is built for the final size in device pixels (DPI, scale, rotation),
    ** so the icon is crisp at any zoom. An aligned box lands on whole pixels — the edges are sharp.
    */
    void paintIcon(const ui::Node& node, const PaintState& state, SceneContext& context)
    {
        const vector::Shape* shape = iconShape(node, context.resources.images);
        const Affine2D& t = state.transform;
        float device = std::sqrt(std::fabs(t.a * t.d - t.b * t.c));
        Rect box = contentRect(node);
        float unit = 1.0f;

        if (shape == nullptr || box.width <= 0.0f || box.height <= 0.0f || device <= 0.0f)
            return;
        text::GlyphKey key{nullptr, quarterPixels(box.height * device), quarterPixels(box.width * device), 0, shape->id};
        ShapeRaster source(*shape, key.quarterSize / 4.0f, key.glyph / 4.0f);
        const text::AtlasEntry* entry = context.resources.atlas.raster(key, source);
        if (entry == nullptr)
            return;
        GpuInstance instance = placeInstance(state, Rect{0.0f, 0.0f, 1.0f, 1.0f}, context.scale, unit);
        float atlasSize = static_cast<float>(context.resources.atlas.size());
        instance.kind = KIND_GLYPH;
        instance.fill = colorPackPremultiplied(node.computed.color);
        placeRaster(instance, *entry, box, state, device);
        instance.extra[0] = entry->x / atlasSize;
        instance.extra[1] = entry->y / atlasSize;
        instance.extra[2] = (entry->x + entry->width) / atlasSize;
        instance.extra[3] = (entry->y + entry->height) / atlasSize;
        context.scene.instances.push_back(instance);
    }
}
