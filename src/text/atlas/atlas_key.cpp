#include "gran/text/glyph_atlas.hpp"

#include <cstdint>

namespace gran::text
{
    // Поля равны попарно / The fields are equal pairwise
    bool GlyphKey::operator==(const GlyphKey& other) const noexcept
    {
        return face == other.face && glyph == other.glyph && quarterSize == other.quarterSize
            && subpixel == other.subpixel && shape == other.shape;
    }

    // FNV-1a по адресу начертания и полям ключа / FNV-1a over the face address and the key fields
    std::size_t GlyphKeyHash::operator()(const GlyphKey& key) const noexcept
    {
        std::uint64_t hash = 1469598103934665603ull;
        std::uint64_t parts[5] = {reinterpret_cast<std::uintptr_t>(key.face), key.glyph, key.quarterSize, key.subpixel,
            key.shape};

        for (std::uint64_t part : parts)
        {
            hash ^= part;
            hash *= 1099511628211ull;
        }
        return static_cast<std::size_t>(hash);
    }
}
