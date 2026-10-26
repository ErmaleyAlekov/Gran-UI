#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    // Знаковое 16-битное через беззнаковое / A signed 16-bit value through the unsigned one
    std::int16_t FontReader::i16(std::size_t offset) const noexcept
    {
        return static_cast<std::int16_t>(u16(offset));
    }

    // Беззнаковое 32-битное big-endian / An unsigned 32-bit big-endian value
    std::uint32_t FontReader::u32(std::size_t offset) const noexcept
    {
        if (!contains(offset, 4))
            return 0;
        return (static_cast<std::uint32_t>(_data[offset]) << 24) | (static_cast<std::uint32_t>(_data[offset + 1]) << 16)
            | (static_cast<std::uint32_t>(_data[offset + 2]) << 8) | _data[offset + 3];
    }
}
