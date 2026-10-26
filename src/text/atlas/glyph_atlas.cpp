#include "gran/text/glyph_atlas.hpp"
#include "gran/text/glyph_rasterizer.hpp"

#include <algorithm>

namespace gran::text
{
    namespace
    {
        // Растр глифа по ключу (пустой контур — false) / The glyph raster by the key (an empty outline — false)
        bool rasterizeKey(const GlyphKey& key, GlyphBitmap& bitmap)
        {
            Outline outline;
            float scale = static_cast<float>(key.quarterSize) / 4.0f / key.face->metrics().unitsPerEm;
            Affine2D toPixels;

            toPixels.a = scale;
            toPixels.d = -scale;
            toPixels.tx = static_cast<float>(key.subpixel) / 4.0f;
            if (!key.face->outline(key.glyph, outline))
                return false;
            return rasterizeOutline(outline, toPixels, bitmap);
        }
    }

    // Пустой атлас; грязная область пуста (left > right) / An empty atlas; the dirty area is empty (left > right)
    GlyphAtlas::GlyphAtlas(std::uint32_t size)
        : _pixels(static_cast<std::size_t>(size) * size, 0), _size(size), _dirtyLeft(size), _dirtyTop(size)
    {
    }

    // Найденный глиф — сразу; новый растеризуется и вставляется; пустой запоминается с нулевой шириной
    // A found glyph — at once; a new one is rasterized and inserted; an empty one is remembered with zero width
    const AtlasEntry* GlyphAtlas::glyph(const GlyphKey& key)
    {
        auto found = _entries.find(key);
        GlyphBitmap bitmap;

        if (found != _entries.end())
        {
            if (found->second.width == 0)
                return nullptr;
            return &found->second;
        }
        if (!rasterizeKey(key, bitmap) || bitmap.width <= 0)
        {
            _entries[key] = AtlasEntry{};
            return nullptr;
        }
        return insert(key, bitmap);
    }

    // Изменённая область и её сброс / The changed area and clearing it
    bool GlyphAtlas::takeDirty(std::uint32_t& x, std::uint32_t& y, std::uint32_t& width, std::uint32_t& height)
    {
        if (_dirtyRight <= _dirtyLeft || _dirtyBottom <= _dirtyTop)
            return false;
        x = _dirtyLeft;
        y = _dirtyTop;
        width = _dirtyRight - _dirtyLeft;
        height = _dirtyBottom - _dirtyTop;
        _dirtyLeft = _size;
        _dirtyTop = _size;
        _dirtyRight = 0;
        _dirtyBottom = 0;
        return true;
    }

    // Сторона атласа / The atlas side
    std::uint32_t GlyphAtlas::size() const noexcept
    {
        return _size;
    }
}
