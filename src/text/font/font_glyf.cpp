#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    namespace
    {
        // Флаги точек простого глифа / The simple glyph point flags
        constexpr std::uint8_t ON_CURVE = 0x01;
        constexpr std::uint8_t X_SHORT = 0x02;
        constexpr std::uint8_t Y_SHORT = 0x04;
        constexpr std::uint8_t REPEAT = 0x08;
        constexpr std::uint8_t X_SAME = 0x10;
        constexpr std::uint8_t Y_SAME = 0x20;

        // Позиция чтения внутри глифа и его конец / The read position inside the glyph and its end
        struct GlyphCursor
        {
            const FontReader& reader;
            std::size_t position;
            std::size_t end;
        };

        // Концы контуров: строго возрастают; возвращает число точек (0 — данные испорчены)
        // The contour ends: strictly increasing; returns the point count (0 — the data is damaged)
        std::size_t readContours(GlyphCursor& cursor, std::size_t contours, std::size_t base, Outline& outline)
        {
            std::size_t previous = 0;

            // Концы контуров (по 2 байта) должны поместиться в глиф до чтения: иначе их место — в чужом глифе
            // The contour ends (2 bytes each) must fit in the glyph before reading: otherwise their place is in a foreign glyph
            if (contours == 0 || cursor.position > cursor.end || contours > (cursor.end - cursor.position) / 2)
                return 0;
            for (std::size_t index = 0; index < contours; ++index)
            {
                std::size_t last = cursor.reader.u16(cursor.position + index * 2);
                if (index > 0 && last <= previous)
                    return 0;
                previous = last;
                outline.contourEnds.push_back(static_cast<std::uint16_t>(base + last));
            }
            cursor.position += contours * 2;
            if (previous + 1 > 65535 - base)
                return 0;
            return previous + 1;
        }

        /*
        ** Пропуск инструкций хинтинга: длина (2 байта) и сами инструкции должны лежать внутри глифа.
        ** Без проверки длина читалась из следующего глифа на глифе, обрезанном после концов контуров.
        **
        ** Skipping the hinting instructions: the length (2 bytes) and the instructions themselves must lie inside the glyph.
        ** Without the check the length was read from the next glyph on a glyph cut right after the contour ends.
        */
        bool skipInstructions(GlyphCursor& cursor)
        {
            if (cursor.end - cursor.position < 2)
                return false;
            std::size_t length = cursor.reader.u16(cursor.position);
            cursor.position += 2;
            if (length > cursor.end - cursor.position)
                return false;
            cursor.position += length;
            return true;
        }

        // Флаги с повторами; false — данные кончились раньше / The flags with repeats; false — the data ended early
        bool readFlags(GlyphCursor& cursor, std::vector<std::uint8_t>& flags)
        {
            std::size_t count = flags.size();

            for (std::size_t index = 0; index < count && cursor.position < cursor.end;)
            {
                std::uint8_t flag = cursor.reader.u8(cursor.position++);
                std::size_t repeat = 0;
                if ((flag & REPEAT) != 0 && cursor.position < cursor.end)
                    repeat = cursor.reader.u8(cursor.position++);
                for (std::size_t copy = 0; copy <= repeat && index < count; ++copy)
                    flags[index++] = flag;
                if (index == count)
                    return true;
            }
            return false;
        }

        /*
        ** Координаты одной оси: короткие (байт со знаком из флага), «та же» (дельта 0)
        ** или длинные (int16). Значения — накопленные дельты; ось Y задаётся vertical.
        **
        ** The coordinates of one axis: short (a byte with the sign from the flag), "same" (delta 0)
        ** or long (int16). The values are accumulated deltas; the Y axis is chosen by vertical.
        */
        bool readAxis(GlyphCursor& cursor, const std::vector<std::uint8_t>& flags, std::vector<Vec2>& points, bool vertical)
        {
            std::uint8_t shortBit = X_SHORT;
            std::uint8_t sameBit = X_SAME;
            float value = 0.0f;

            if (vertical)
            {
                shortBit = Y_SHORT;
                sameBit = Y_SAME;
            }
            for (std::size_t index = 0; index < flags.size(); ++index)
            {
                if ((flags[index] & shortBit) != 0)
                {
                    float delta = cursor.reader.u8(cursor.position++);
                    if ((flags[index] & sameBit) == 0)
                        delta = -delta;
                    value += delta;
                }
                else if ((flags[index] & sameBit) == 0)
                {
                    value += cursor.reader.i16(cursor.position);
                    cursor.position += 2;
                }
                if (vertical)
                    points[index].y = value;
                else
                    points[index].x = value;
            }
            return cursor.position <= cursor.end;
        }
    }

    // Простой глиф: концы контуров, пропуск инструкций хинтинга, флаги, X, Y
    // A simple glyph: the contour ends, skipping the hinting instructions, the flags, X, Y
    bool readSimpleGlyph(const FontReader& reader, std::uint32_t offset, std::uint32_t end, Outline& outline)
    {
        GlyphCursor cursor{reader, offset + 10u, end};
        std::size_t base = outline.points.size();
        std::size_t count = readContours(cursor, static_cast<std::size_t>(reader.i16(offset)), base, outline);

        if (count == 0 || !skipInstructions(cursor))
            return false;
        std::vector<std::uint8_t> flags(count);
        std::vector<Vec2> points(count);
        if (!readFlags(cursor, flags) || !readAxis(cursor, flags, points, false) || !readAxis(cursor, flags, points, true))
            return false;
        for (std::size_t index = 0; index < count; ++index)
        {
            outline.points.push_back(points[index]);
            outline.onCurve.push_back(static_cast<std::uint8_t>(flags[index] & ON_CURVE));
        }
        return true;
    }
}
