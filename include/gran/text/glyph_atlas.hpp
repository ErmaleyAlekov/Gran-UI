#ifndef GRAN_TEXT_GLYPH_ATLAS_HPP
#define GRAN_TEXT_GLYPH_ATLAS_HPP

#include "gran/text/glyph_rasterizer.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gran::text
{
    // Глиф в атласе: место в текстуре и смещение от точки пера / A glyph in the atlas: the texture place and the offset from the pen point
    struct AtlasEntry
    {
        std::int16_t left = 0;
        std::int16_t top = 0;
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        std::uint16_t width = 0;
        std::uint16_t height = 0;
    };

    // Ключ растра: начертание, глиф, размер в ¼ px, подпиксельный сдвиг 0..3 / A raster key: the face, the glyph, the size in ¼ px, the subpixel shift 0..3
    struct GlyphKey
    {
        const FontFace* face = nullptr;
        std::uint16_t glyph = 0;
        std::uint16_t quarterSize = 0;
        std::uint8_t subpixel = 0;

        // ---- atlas_key.cpp ----
        bool operator==(const GlyphKey& other) const noexcept;
    };

    // Хеш ключа (FNV-1a по полям) / The key hash (FNV-1a over the fields)
    struct GlyphKeyHash
    {
        std::size_t operator()(const GlyphKey& key) const noexcept;
    };

    /*
    ** Атлас покрытия R8 на CPU с копией для GPU: глифы растеризуются по требованию
    ** и укладываются полками. Переполнение — рост вдвое (до 4096), затем полный сброс
    ** с новым поколением: рендерер заново загрузит текстуру и перерисует кадр.
    **
    ** An R8 coverage atlas on the CPU with a copy for the GPU: glyphs are rasterized on demand
    ** and packed into shelves. An overflow means doubling (up to 4096), then a full reset
    ** with a new generation: the renderer uploads the texture again and redraws the frame.
    */
    class GlyphAtlas
    {
    public:
        // ---- glyph_atlas.cpp ----
        explicit GlyphAtlas(std::uint32_t size = 1024);

        // Растр глифа; nullptr — глиф пуст (пробел) / The glyph raster; nullptr — the glyph is empty (a space)
        const AtlasEntry* glyph(const GlyphKey& key);

        // Изменённая область с прошлого вызова; false — изменений нет / The changed area since the last call; false — no changes
        bool takeDirty(std::uint32_t& x, std::uint32_t& y, std::uint32_t& width, std::uint32_t& height);

        // Сторона квадрата атласа и пиксели / The atlas square side and the pixels
        std::uint32_t size() const noexcept;
        const std::uint8_t* pixels() const noexcept;

        // ---- atlas_packing.cpp ----

        // Номер поколения: меняется при росте и сбросе / The generation number: changes on growing and resetting
        std::uint32_t generation() const noexcept;

    private:
        // ---- atlas_insert.cpp ----
        // Новая запись: место, копия пикселей, грязная область / A new entry: a place, a pixel copy, the dirty area
        const AtlasEntry* insert(const GlyphKey& key, const GlyphBitmap& bitmap);

        // ---- atlas_packing.cpp ----
        // Место под растр на полке; false — места нет / A place for a raster on a shelf; false — no room
        bool pack(std::uint32_t width, std::uint32_t height, std::uint32_t& x, std::uint32_t& y);

        // Рост вдвое с переносом строк пикселей / Doubling with moving the pixel rows
        bool grow();

        // Полный сброс: записи, полки, пиксели / A full reset: the entries, the shelves, the pixels
        void reset();

        // Полка: верх, высота, занятая ширина / A shelf: the top, the height, the used width
        struct Shelf
        {
            std::uint32_t y;
            std::uint32_t height;
            std::uint32_t used;
        };

        std::unordered_map<GlyphKey, AtlasEntry, GlyphKeyHash> _entries;
        std::vector<Shelf> _shelves;
        std::vector<std::uint8_t> _pixels;
        std::uint32_t _size;
        std::uint32_t _generation = 1;
        std::uint32_t _dirtyLeft;
        std::uint32_t _dirtyTop;
        std::uint32_t _dirtyRight = 0;
        std::uint32_t _dirtyBottom = 0;
    };
}

#endif
