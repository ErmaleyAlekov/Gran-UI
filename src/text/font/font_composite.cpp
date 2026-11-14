#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    namespace
    {
        // Предел вложенности компонентов: защита от циклов в испорченном шрифте / The component nesting limit: a guard against cycles in a damaged font
        constexpr int MAX_DEPTH = 8;

        /*
        ** Предел посещённых глифов на один контур. Одна глубина не спасает: глиф из k компонент, каждая из которых
        ** ссылается на него же, дал бы k^8 вызовов (зависание на испорченном шрифте). Настоящие составные глифы
        ** посещают единицы глифов.
        **
        ** The limit of visited glyphs per outline. The depth alone does not help: a glyph of k components, each
        ** referring to the glyph itself, would give k^8 calls (a hang on a damaged font). Real composite glyphs
        ** visit a handful of glyphs.
        */
        constexpr std::size_t MAX_VISITS = 1024;

        // Откуда читать глифы; glyphCount — из maxp; visits — счётчик посещений на один контур
        // Where to read the glyphs from; glyphCount is from maxp; visits is the visit counter per outline
        struct GlyphSource
        {
            const FontReader& reader;
            const FaceTables& tables;
            bool longLoca;
            std::uint16_t glyphCount;
            std::size_t& visits;
        };

        // Позиция чтения внутри составного глифа и его конец / The read position inside a composite glyph and its end
        struct ComponentCursor
        {
            std::size_t position;
            std::size_t end;
        };

        bool readGlyph(const GlyphSource& source, std::uint16_t glyph, int depth, Outline& outline);

        /*
        ** Один компонент: глиф, смещение (только XY-значения; сопоставление точек редкое
        ** и даёт нулевое смещение), матрица. Точки компонента преобразуются и дописываются.
        ** Запись целиком (по флагам) должна лежать внутри глифа, а индекс глифа — быть меньше numGlyphs:
        ** иначе false, и составной глиф считается испорченным. Пустой компонент (пробел) пропускается.
        **
        ** One component: the glyph, the offset (XY values only; point matching is rare
        ** and gives a zero offset), the matrix. The component points are transformed and appended.
        ** The whole record (by the flags) must lie inside the glyph and the glyph index must be below numGlyphs:
        ** otherwise false, and the composite glyph is treated as damaged. An empty component (a space) is skipped.
        */
        bool readComponent(const GlyphSource& source, ComponentCursor& cursor, int depth, Outline& outline,
            std::uint16_t& flags)
        {
            const FontReader& reader = source.reader;
            Affine2D transform;
            Outline part;

            flags = reader.u16(cursor.position);
            std::uint16_t glyph = reader.u16(cursor.position + 2);
            if (componentRecordSize(flags) > cursor.end - cursor.position || glyph >= source.glyphCount)
                return false;
            cursor.position = readComponentOffset(reader, cursor.position + 4, flags, transform);
            cursor.position = readComponentTransform(reader, cursor.position, flags, transform);
            std::size_t base = outline.points.size();
            if (!readGlyph(source, glyph, depth + 1, part) || part.points.size() > 65535 - base)
                return true;
            for (std::size_t index = 0; index < part.points.size(); ++index)
            {
                outline.points.push_back(affineApply(transform, part.points[index]));
                outline.onCurve.push_back(part.onCurve[index]);
            }
            for (std::uint16_t end : part.contourEnds)
                outline.contourEnds.push_back(static_cast<std::uint16_t>(base + end));
            return true;
        }

        // Простой или составной глиф с пределами глубины и посещений / A simple or composite glyph with the depth and visit limits
        bool readGlyph(const GlyphSource& source, std::uint16_t glyph, int depth, Outline& outline)
        {
            std::uint32_t offset = 0;
            std::uint32_t end = 0;
            std::uint16_t flags = COMPONENT_MORE_COMPONENTS;

            if (depth > MAX_DEPTH || ++source.visits > MAX_VISITS || glyph >= source.glyphCount
                || !glyphLocation(source.reader, source.tables, source.longLoca, glyph, offset, end))
                return false;
            if (source.reader.i16(offset) >= 0)
                return readSimpleGlyph(source.reader, offset, end, outline);
            // glyphLocation гарантирует end − offset ≥ 10: вычитания ниже без переноса
            // glyphLocation guarantees end − offset ≥ 10: the subtractions below do not wrap
            ComponentCursor cursor{static_cast<std::size_t>(offset) + 10, end};
            while ((flags & COMPONENT_MORE_COMPONENTS) != 0)
            {
                if (cursor.end - cursor.position < 4 || !readComponent(source, cursor, depth, outline, flags))
                    return false;
            }
            return !outline.points.empty();
        }
    }

    // Контур глифа; при ошибке данных — пустой контур (глиф не рисуется), без исключения
    // The glyph outline; on a data error — an empty outline (the glyph is not drawn), without an exception
    bool FontFace::outline(std::uint16_t glyph, Outline& result) const
    {
        FontReader reader(_file->data(), _file->size());
        std::size_t visits = 0;
        GlyphSource source{reader, _tables, _longLoca, _glyphCount, visits};

        result = Outline{};
        if (!readGlyph(source, glyph, 0, result))
        {
            result = Outline{};
            return false;
        }
        return true;
    }
}
