#include "gran/text/glyph_atlas.hpp"
#include "gran/text/glyph_rasterizer.hpp"

#include <algorithm>

namespace gran::text
{
    /*
    ** Место на полке (нет места — рост, затем полный сброс), копия строк растра и
    ** расширение грязной области. Растр не больше 2048 (предел растеризатора) меньше
    ** наименьшего атласа после роста, поэтому цикл всегда завершается.
    **
    ** A place on a shelf (no room — growing, then a full reset), a copy of the raster rows and
    ** widening of the dirty area. A raster of at most 2048 (the rasterizer limit) is smaller
    ** than the atlas after growing, so the loop always ends.
    */
    const AtlasEntry* GlyphAtlas::insert(const GlyphKey& key, const GlyphBitmap& bitmap)
    {
        std::uint32_t width = static_cast<std::uint32_t>(bitmap.width);
        std::uint32_t height = static_cast<std::uint32_t>(bitmap.height);
        std::uint32_t x = 0;
        std::uint32_t y = 0;

        while (!pack(width, height, x, y))
        {
            if (!grow())
                reset();
        }
        for (std::uint32_t row = 0; row < height; ++row)
            std::copy_n(bitmap.coverage.begin() + static_cast<std::ptrdiff_t>(row) * width, width,
                _pixels.begin() + static_cast<std::ptrdiff_t>(y + row) * _size + x);
        _dirtyLeft = std::min(_dirtyLeft, x);
        _dirtyTop = std::min(_dirtyTop, y);
        _dirtyRight = std::max(_dirtyRight, x + width);
        _dirtyBottom = std::max(_dirtyBottom, y + height);
        AtlasEntry& entry = _entries[key];
        entry = AtlasEntry{static_cast<std::int16_t>(bitmap.left), static_cast<std::int16_t>(bitmap.top),
            static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(height)};
        return &entry;
    }
}
