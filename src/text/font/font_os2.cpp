#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    namespace
    {
        // Оценка подтаблицы cmap: 2 — формат 12, 1 — формат 4 Юникода, 0 — не подходит
        // A cmap subtable score: 2 — format 12, 1 — a Unicode format 4, 0 — unsuitable
        int cmapScore(std::uint16_t platform, std::uint16_t encoding, std::uint16_t format) noexcept
        {
            bool unicode = platform == 0 || (platform == 3 && (encoding == 1 || encoding == 10));

            if (!unicode)
                return 0;
            if (format == 12)
                return 2;
            if (format == 4)
                return 1;
            return 0;
        }

        // Подтаблица целиком внутри cmap (формат 4: длина в заголовке, 12: группы по 12 байт)
        // The subtable entirely inside cmap (format 4: the length in the header, 12: groups of 12 bytes)
        bool subtableFits(const FontReader& reader, const TableSpan& cmap, std::uint32_t offset, std::uint16_t format)
        {
            std::size_t length = reader.u16(offset + 2);

            if (format == 12)
                length = 16 + static_cast<std::size_t>(reader.u32(offset + 12)) * 12;
            return offset >= cmap.offset && offset - cmap.offset <= cmap.length
                && length <= cmap.length - (offset - cmap.offset);
        }
    }

    /*
    ** OS/2 уточняет метрики: насыщенность, курсив (бит 0 fsSelection), высоты. С флагом
    ** USE_TYPO_METRICS (бит 7) строка измеряется типографскими значениями, иначе — hhea
    ** (как в браузерах на macOS и Linux: одинаковая высота строки на всех ОС).
    **
    ** OS/2 refines the metrics: the weight, italic (fsSelection bit 0), the heights. With the
    ** USE_TYPO_METRICS flag (bit 7) the line is measured by the typographic values, otherwise by hhea
    ** (as in browsers on macOS and Linux: the same line height on all OSes).
    */
    void FaceBuilder::readOs2(FontFace& face)
    {
        FontReader reader(face._file->data(), face._file->size());
        const TableSpan& os2 = face._tables.os2;
        FontMetrics& metrics = face._metrics;

        metrics.xHeight = metrics.unitsPerEm * 0.5f;
        metrics.capHeight = metrics.unitsPerEm * 0.7f;
        if (os2.length < 78)
            return;
        std::uint16_t selection = reader.u16(os2.offset + 62);
        metrics.weight = reader.u16(os2.offset + 4);
        metrics.italic = (selection & 0x0001) != 0;
        if ((selection & 0x0080) != 0)
        {
            metrics.ascender = reader.i16(os2.offset + 68);
            metrics.descender = reader.i16(os2.offset + 70);
            metrics.lineGap = reader.i16(os2.offset + 72);
        }
        if (reader.u16(os2.offset) >= 2 && os2.length >= 90)
        {
            metrics.xHeight = reader.i16(os2.offset + 86);
            metrics.capHeight = reader.i16(os2.offset + 88);
        }
    }

    // Лучшая подходящая подтаблица; её границы проверяются здесь, при поиске — уже нет
    // The best suitable subtable; its bounds are checked here, the lookups no longer check them
    void FaceBuilder::selectCmap(FontFace& face, const std::string& source)
    {
        FontReader reader(face._file->data(), face._file->size());
        const TableSpan& cmap = face._tables.cmap;
        std::uint16_t count = reader.u16(cmap.offset + 2);
        int best = 0;

        for (std::uint16_t index = 0; index < count && cmap.length >= 4 + (index + 1u) * 8u; ++index)
        {
            std::size_t record = cmap.offset + 4 + static_cast<std::size_t>(index) * 8;
            std::uint32_t offset = cmap.offset + reader.u32(record + 4);
            std::uint16_t format = reader.u16(offset);
            int score = cmapScore(reader.u16(record), reader.u16(record + 2), format);
            if (score > best && subtableFits(reader, cmap, offset, format))
            {
                best = score;
                face._cmapSubtable = offset;
                face._cmapFormat = format;
            }
        }
        if (best == 0)
            throwBadFont(source, "cmap");
    }
}
