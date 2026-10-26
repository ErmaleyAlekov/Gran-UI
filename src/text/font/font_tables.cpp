#include "gran/text/internal/font_parts.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

namespace gran::text
{
    namespace
    {
        // Тег таблицы из четырёх символов / A table tag from four characters
        constexpr std::uint32_t tag(char a, char b, char c, char d)
        {
            return (static_cast<std::uint32_t>(a) << 24) | (static_cast<std::uint32_t>(b) << 16)
                | (static_cast<std::uint32_t>(c) << 8) | static_cast<std::uint32_t>(d);
        }

        // Таблица по тегу: куда записать её расположение и обязательна ли она
        // A table by tag: where to store its location and whether it is required
        struct TableSlot
        {
            std::uint32_t tag;
            TableSpan FaceTables::* span;
            const char* name;
            bool required;
        };

        const TableSlot SLOTS[] = {
            {tag('h', 'e', 'a', 'd'), &FaceTables::head, "head", true}, {tag('h', 'h', 'e', 'a'), &FaceTables::hhea, "hhea", true},
            {tag('m', 'a', 'x', 'p'), &FaceTables::maxp, "maxp", true}, {tag('h', 'm', 't', 'x'), &FaceTables::hmtx, "hmtx", true},
            {tag('l', 'o', 'c', 'a'), &FaceTables::loca, "loca", true}, {tag('g', 'l', 'y', 'f'), &FaceTables::glyf, "glyf", true},
            {tag('c', 'm', 'a', 'p'), &FaceTables::cmap, "cmap", true}, {tag('O', 'S', '/', '2'), &FaceTables::os2, "OS/2", false},
            {tag('n', 'a', 'm', 'e'), &FaceTables::name, "name", false}, {tag('k', 'e', 'r', 'n'), &FaceTables::kern, "kern", false},
            {tag('G', 'P', 'O', 'S'), &FaceTables::gpos, "GPOS", false}};

        // Начало каталога таблиц: 0 у TTF, смещение начертания index у TTC / The table directory start: 0 for a TTF, the face index offset for a TTC
        std::uint32_t directoryStart(const FontReader& reader, std::uint32_t index, const std::string& source)
        {
            if (reader.u32(0) != tag('t', 't', 'c', 'f'))
                return 0;
            if (index >= reader.u32(8) || !reader.contains(12 + static_cast<std::size_t>(index) * 4, 4))
                throwBadFont(source, "TTC index");
            return reader.u32(12 + static_cast<std::size_t>(index) * 4);
        }
    }

    namespace
    {
        // Одна запись каталога: таблица внутри файла и, если она нужна, её место / One directory record: the table inside the file and, if needed, its place
        void readRecord(FaceTables& tables, const FontReader& reader, std::size_t record, const std::string& source)
        {
            TableSpan span{reader.u32(record + 8), reader.u32(record + 12)};

            if (!reader.contains(span.offset, span.length))
                throwBadFont(source, "table bounds");
            for (const TableSlot& slot : SLOTS)
            {
                if (slot.tag == reader.u32(record))
                    tables.*slot.span = span;
            }
        }
    }

    /*
    ** Каталог: версия 0x00010000 или 'true' (контуры glyf). CFF ('OTTO') пока не поддерживается.
    ** Каждая таблица обязана лежать внутри файла; обязательные таблицы должны быть.
    **
    ** The directory: version 0x00010000 or 'true' (glyf outlines). CFF ('OTTO') is not supported yet.
    ** Every table must lie inside the file; the required tables must be present.
    */
    void FaceBuilder::readDirectory(FontFace& face, std::uint32_t index, const std::string& source)
    {
        FontReader reader(face._file->data(), face._file->size());
        std::uint32_t base = directoryStart(reader, index, source);
        std::uint32_t version = reader.u32(base);
        std::uint16_t count = reader.u16(base + 4);

        if (version != 0x00010000u && version != tag('t', 'r', 'u', 'e'))
            throwBadFont(source, "outline format (CFF / OTTO is not supported)");
        if (!reader.contains(base + 12, static_cast<std::size_t>(count) * 16))
            throwBadFont(source, "table directory");
        for (std::uint16_t table = 0; table < count; ++table)
            readRecord(face._tables, reader, base + 12 + static_cast<std::size_t>(table) * 16, source);
        for (const TableSlot& slot : SLOTS)
        {
            if (slot.required && (face._tables.*slot.span).length == 0)
                throw GranException(ErrorCode::FONT, i18n::format(i18n::Message::FONT_MISSING_TABLE, {slot.name}));
        }
    }

    // Заголовки и проверка размеров hmtx и loca относительно числа глифов / The headers and a check of the hmtx and loca sizes against the glyph count
    void FaceBuilder::readHeader(FontFace& face, const std::string& source)
    {
        FontReader reader(face._file->data(), face._file->size());
        const FaceTables& tables = face._tables;

        if (tables.head.length < 54 || tables.hhea.length < 36 || tables.maxp.length < 6)
            throwBadFont(source, "head/hhea/maxp size");
        face._metrics.unitsPerEm = reader.u16(tables.head.offset + 18);
        face._longLoca = reader.i16(tables.head.offset + 50) != 0;
        face._metrics.ascender = reader.i16(tables.hhea.offset + 4);
        face._metrics.descender = reader.i16(tables.hhea.offset + 6);
        face._metrics.lineGap = reader.i16(tables.hhea.offset + 8);
        face._horizontalMetrics = reader.u16(tables.hhea.offset + 34);
        face._glyphCount = reader.u16(tables.maxp.offset + 4);
        std::size_t locaEntry = 2;
        if (face._longLoca)
            locaEntry = 4;
        if (face._metrics.unitsPerEm < 16 || face._horizontalMetrics == 0 || face._horizontalMetrics > face._glyphCount
            || tables.hmtx.length < static_cast<std::size_t>(face._horizontalMetrics) * 4
            || tables.loca.length < (static_cast<std::size_t>(face._glyphCount) + 1) * locaEntry)
            throwBadFont(source, "metrics");
    }
}
