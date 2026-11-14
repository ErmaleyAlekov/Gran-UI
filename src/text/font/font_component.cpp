#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    // Байты записи: флаги и глиф (4), смещение (2 или 4), матрица (0, 2, 4 или 8)
    // The record bytes: the flags and the glyph (4), the offset (2 or 4), the matrix (0, 2, 4 or 8)
    std::size_t componentRecordSize(std::uint16_t flags) noexcept
    {
        std::size_t size = 4 + 2;

        if ((flags & COMPONENT_ARGS_ARE_WORDS) != 0)
            size += 2;
        if ((flags & COMPONENT_HAS_SCALE) != 0)
            size += 2;
        else if ((flags & COMPONENT_HAS_XY_SCALE) != 0)
            size += 4;
        else if ((flags & COMPONENT_HAS_TWO_BY_TWO) != 0)
            size += 8;
        return size;
    }

    // Смещение компонента: два int16 или два int8; без флага XY — сопоставление точек (смещение 0)
    // The component offset: two int16 or two int8; without the XY flag — point matching (a zero offset)
    std::size_t readComponentOffset(const FontReader& reader, std::size_t position, std::uint16_t flags,
        Affine2D& transform)
    {
        bool xy = (flags & COMPONENT_ARGS_ARE_XY) != 0;

        if ((flags & COMPONENT_ARGS_ARE_WORDS) != 0)
        {
            if (xy)
            {
                transform.tx = reader.i16(position);
                transform.ty = reader.i16(position + 2);
            }
            return position + 4;
        }
        if (xy)
        {
            transform.tx = static_cast<std::int8_t>(reader.u8(position));
            transform.ty = static_cast<std::int8_t>(reader.u8(position + 1));
        }
        return position + 2;
    }

    // Матрица компонента в формате F2Dot14 / The component matrix in F2Dot14
    std::size_t readComponentTransform(const FontReader& reader, std::size_t position, std::uint16_t flags,
        Affine2D& transform)
    {
        const float unit = 1.0f / 16384.0f;

        if ((flags & COMPONENT_HAS_SCALE) != 0)
        {
            transform.a = reader.i16(position) * unit;
            transform.d = transform.a;
            return position + 2;
        }
        if ((flags & COMPONENT_HAS_XY_SCALE) != 0)
        {
            transform.a = reader.i16(position) * unit;
            transform.d = reader.i16(position + 2) * unit;
            return position + 4;
        }
        if ((flags & COMPONENT_HAS_TWO_BY_TWO) != 0)
        {
            transform.a = reader.i16(position) * unit;
            transform.b = reader.i16(position + 2) * unit;
            transform.c = reader.i16(position + 4) * unit;
            transform.d = reader.i16(position + 6) * unit;
            return position + 8;
        }
        return position;
    }
}
