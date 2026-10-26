#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    namespace
    {
        // Флаги компонентов составного глифа / The composite glyph component flags
        constexpr std::uint16_t ARGS_ARE_WORDS = 0x0001;
        constexpr std::uint16_t ARGS_ARE_XY = 0x0002;
        constexpr std::uint16_t HAS_SCALE = 0x0008;
        constexpr std::uint16_t MORE_COMPONENTS = 0x0020;
        constexpr std::uint16_t HAS_XY_SCALE = 0x0040;
        constexpr std::uint16_t HAS_TWO_BY_TWO = 0x0080;

        // Предел вложенности компонентов: защита от циклов в испорченном шрифте / The component nesting limit: a guard against cycles in a damaged font
        constexpr int MAX_DEPTH = 8;

        // Откуда читать глифы / Where to read the glyphs from
        struct GlyphSource
        {
            const FontReader& reader;
            const FaceTables& tables;
            bool longLoca;
        };

        // Матрица компонента в формате F2Dot14 и смещение / The component matrix in F2Dot14 and the offset
        std::size_t readTransform(const FontReader& reader, std::size_t position, std::uint16_t flags, Affine2D& transform)
        {
            const float unit = 1.0f / 16384.0f;

            if ((flags & HAS_SCALE) != 0)
            {
                transform.a = reader.i16(position) * unit;
                transform.d = transform.a;
                return position + 2;
            }
            if ((flags & HAS_XY_SCALE) != 0)
            {
                transform.a = reader.i16(position) * unit;
                transform.d = reader.i16(position + 2) * unit;
                return position + 4;
            }
            if ((flags & HAS_TWO_BY_TWO) != 0)
            {
                transform.a = reader.i16(position) * unit;
                transform.b = reader.i16(position + 2) * unit;
                transform.c = reader.i16(position + 4) * unit;
                transform.d = reader.i16(position + 6) * unit;
                return position + 8;
            }
            return position;
        }

        // Смещение компонента: два int16 или два int8; без флага XY — сопоставление точек (смещение 0)
        // The component offset: two int16 or two int8; without the XY flag — point matching (a zero offset)
        std::size_t readOffset(const FontReader& reader, std::size_t position, std::uint16_t flags, Affine2D& transform)
        {
            bool xy = (flags & ARGS_ARE_XY) != 0;

            if ((flags & ARGS_ARE_WORDS) != 0)
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

        bool readGlyph(const GlyphSource& source, std::uint16_t glyph, int depth, Outline& outline);

        /*
        ** Один компонент: глиф, смещение (только XY-значения; сопоставление точек редкое
        ** и даёт нулевое смещение), матрица. Точки компонента преобразуются и дописываются.
        **
        ** One component: the glyph, the offset (XY values only; point matching is rare
        ** and gives a zero offset), the matrix. The component points are transformed and appended.
        */
        std::size_t readComponent(const GlyphSource& source, std::size_t position, int depth, Outline& outline,
            std::uint16_t& flags)
        {
            const FontReader& reader = source.reader;
            Affine2D transform;
            Outline part;
            std::uint16_t glyph = reader.u16(position + 2);

            flags = reader.u16(position);
            position = readOffset(reader, position + 4, flags, transform);
            position = readTransform(reader, position, flags, transform);
            std::size_t base = outline.points.size();
            if (!readGlyph(source, glyph, depth + 1, part) || part.points.size() > 65535 - base)
                return position;
            for (std::size_t index = 0; index < part.points.size(); ++index)
            {
                outline.points.push_back(affineApply(transform, part.points[index]));
                outline.onCurve.push_back(part.onCurve[index]);
            }
            for (std::uint16_t end : part.contourEnds)
                outline.contourEnds.push_back(static_cast<std::uint16_t>(base + end));
            return position;
        }

        // Простой или составной глиф с ограничением глубины / A simple or composite glyph with a depth limit
        bool readGlyph(const GlyphSource& source, std::uint16_t glyph, int depth, Outline& outline)
        {
            std::uint32_t offset = 0;
            std::uint32_t end = 0;
            std::uint16_t flags = MORE_COMPONENTS;

            if (depth > MAX_DEPTH || !glyphLocation(source.reader, source.tables, source.longLoca, glyph, offset, end))
                return false;
            if (source.reader.i16(offset) >= 0)
                return readSimpleGlyph(source.reader, offset, end, outline);
            std::size_t position = offset + 10;
            while ((flags & MORE_COMPONENTS) != 0 && position + 4 <= end)
                position = readComponent(source, position, depth, outline, flags);
            return !outline.points.empty();
        }
    }

    // Контур глифа; при ошибке данных — пустой контур (глиф не рисуется), без исключения
    // The glyph outline; on a data error — an empty outline (the glyph is not drawn), without an exception
    bool FontFace::outline(std::uint16_t glyph, Outline& result) const
    {
        FontReader reader(_file->data(), _file->size());
        GlyphSource source{reader, _tables, _longLoca};

        result = Outline{};
        if (glyph >= _glyphCount || !readGlyph(source, glyph, 0, result))
        {
            result = Outline{};
            return false;
        }
        return true;
    }
}
