#include "gran/text/glyph_atlas.hpp"
#include "gran/text/glyph_rasterizer.hpp"

#include <algorithm>

namespace gran::text
{
    // Область для загрузки; сброс после успешной передачи / Upload region, cleared after successful transfer
    bool GlyphAtlas::dirtyRegion(std::uint32_t& x, std::uint32_t& y, std::uint32_t& width, std::uint32_t& height) const noexcept
    {
        if (_dirtyRight <= _dirtyLeft || _dirtyBottom <= _dirtyTop)
            return false;
        x = _dirtyLeft;
        y = _dirtyTop;
        width = _dirtyRight - _dirtyLeft;
        height = _dirtyBottom - _dirtyTop;
        return true;
    }

    // Проверяем размеры до упаковки и копирования строк.
    // Validate dimensions before packing and copying rows.
    const AtlasEntry* GlyphAtlas::insert(const GlyphKey& key, const GlyphBitmap& bitmap, bool preserveLayout)
    {
        std::uint32_t width = static_cast<std::uint32_t>(bitmap.width);
        std::uint32_t height = static_cast<std::uint32_t>(bitmap.height);
        std::uint32_t x = 0;
        std::uint32_t y = 0;

        if (bitmap.width <= 0 || width >= MAX_SIZE || bitmap.height <= 0 || height > MAX_SIZE
            || bitmap.left < INT16_MIN || bitmap.left > INT16_MAX || bitmap.top < INT16_MIN || bitmap.top > INT16_MAX
            || bitmap.coverage.size() < static_cast<std::size_t>(width) * height)
            return nullptr;
        while (!pack(width, height, x, y))
        {
            if (preserveLayout)
                return nullptr;
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

    // Как glyph(), но растр даёт источник; пустой растр тоже запоминается / As glyph(), but the source gives the raster; an empty raster is remembered too
    const AtlasEntry* GlyphAtlas::raster(const GlyphKey& key, RasterSource& source, bool preserveLayout)
    {
        auto found = _entries.find(key);
        GlyphBitmap bitmap;

        if (found != _entries.end())
        {
            if (found->second.width == 0)
                return nullptr;
            return &found->second;
        }
        if (!source.rasterize(bitmap) || bitmap.width <= 0)
        {
            _entries[key] = AtlasEntry{};
            return nullptr;
        }
        return insert(key, bitmap, preserveLayout);
    }
}
