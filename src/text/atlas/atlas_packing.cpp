#include "gran/text/glyph_atlas.hpp"

#include <algorithm>

namespace gran::text
{
    namespace
    {
        // Наибольшая сторона атласа (16 МБ на CPU) / The largest atlas side (16 MB on the CPU)
        constexpr std::uint32_t MAX_SIZE = 4096;
    }

    // Поколение атласа / The atlas generation
    std::uint32_t GlyphAtlas::generation() const noexcept
    {
        return _generation;
    }

    /*
    ** Полка подходит, если её высота не меньше растра и не больше чем на четверть выше
    ** (иначе мелкие глифы съели бы место высоких полок); иначе — новая полка снизу.
    ** Между растрами — 1 пиксель пустоты, чтобы линейная выборка не цепляла соседей.
    **
    ** A shelf fits if its height is not below the raster and not more than a quarter higher
    ** (otherwise small glyphs would eat the room of tall shelves); otherwise — a new shelf at the bottom.
    ** There is 1 empty pixel between rasters so linear sampling does not catch the neighbours.
    */
    bool GlyphAtlas::pack(std::uint32_t width, std::uint32_t height, std::uint32_t& x, std::uint32_t& y)
    {
        std::uint32_t bottom = 0;

        for (Shelf& shelf : _shelves)
        {
            bottom = std::max(bottom, shelf.y + shelf.height + 1);
            if (shelf.height >= height && shelf.height <= height + height / 4 + 2 && shelf.used + width + 1 <= _size)
            {
                x = shelf.used;
                y = shelf.y;
                shelf.used += width + 1;
                return true;
            }
        }
        if (bottom + height > _size || width + 1 > _size)
            return false;
        _shelves.push_back(Shelf{bottom, height, width + 1});
        x = 0;
        y = bottom;
        return true;
    }

    // Строки старого атласа переносятся в больший; координаты глифов не меняются
    // The old atlas rows move into the larger one; the glyph coordinates stay the same
    bool GlyphAtlas::grow()
    {
        std::uint32_t larger = _size * 2;

        if (larger > MAX_SIZE)
            return false;
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(larger) * larger, 0);
        for (std::uint32_t row = 0; row < _size; ++row)
            std::copy_n(_pixels.begin() + static_cast<std::ptrdiff_t>(row) * _size, _size,
                pixels.begin() + static_cast<std::ptrdiff_t>(row) * larger);
        _pixels.swap(pixels);
        _size = larger;
        _generation += 1;
        return true;
    }

    // Всё заново: старые записи недействительны, поколение меняется / Everything anew: the old entries are invalid, the generation changes
    void GlyphAtlas::reset()
    {
        _entries.clear();
        _shelves.clear();
        std::fill(_pixels.begin(), _pixels.end(), std::uint8_t{0});
        _generation += 1;
        _dirtyLeft = _size;
        _dirtyTop = _size;
        _dirtyRight = 0;
        _dirtyBottom = 0;
    }

    // Пиксели атласа построчно / The atlas pixels row by row
    const std::uint8_t* GlyphAtlas::pixels() const noexcept
    {
        return _pixels.data();
    }
}
