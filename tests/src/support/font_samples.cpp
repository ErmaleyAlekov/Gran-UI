#include "support/font_samples.hpp"

#include <filesystem>
#include <fstream>

namespace font_samples
{
    namespace
    {
        // Число big-endian (до 8 байт) в конец буфера / A big-endian number (up to 8 bytes) to the buffer end
        void put(Bytes& out, std::uint64_t value, int bytes)
        {
            for (int shift = (bytes - 1) * 8; shift >= 0; shift -= 8)
                out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }

        // Таблицы, кроме loca и glyf: head (длинная loca), hhea (1 метрика), maxp, hmtx, cmap 4 (только 0xFFFF)
        // The tables except loca and glyf: head (the long loca), hhea (1 metric), maxp, hmtx, cmap 4 (0xFFFF only)
        std::vector<Bytes> fixedTables(std::size_t glyphCount)
        {
            Bytes head(54, 0);
            Bytes hhea(36, 0);
            Bytes maxp;
            Bytes hmtx;
            Bytes cmap;

            head[18] = 1000 >> 8;
            head[19] = 1000 & 0xFF;
            head[51] = 1;
            hhea[35] = 1;
            put(maxp, 0x00005000u, 4);
            put(maxp, static_cast<std::uint32_t>(glyphCount), 2);
            put(hmtx, 500, 2);
            put(hmtx, 0, 2);
            put(cmap, 0, 2);
            put(cmap, 1, 2);
            put(cmap, 3, 2);
            put(cmap, 1, 2);
            put(cmap, 12, 4);
            for (std::uint32_t value : {4u, 24u, 0u, 2u, 2u, 0u, 0u, 0xFFFFu, 0u, 0xFFFFu, 1u, 0u})
                put(cmap, value, 2);
            return {head, hhea, maxp, hmtx, cmap};
        }

        // Каталог из семи таблиц и сами таблицы с выравниванием на 4 / The directory of seven tables and the tables aligned to 4
        Bytes assemble(const std::vector<Bytes>& tables)
        {
            static const char* const TAGS[7] = {"head", "hhea", "maxp", "hmtx", "cmap", "loca", "glyf"};
            Bytes out;
            std::uint32_t offset = 12 + 16 * 7;

            put(out, 0x00010000u, 4);
            put(out, 7, 2);
            put(out, 0, 6);
            for (std::size_t index = 0; index < 7; ++index)
            {
                for (int letter = 0; letter < 4; ++letter)
                    out.push_back(static_cast<std::uint8_t>(TAGS[index][letter]));
                put(out, 0, 4);
                put(out, offset, 4);
                put(out, static_cast<std::uint32_t>(tables[index].size()), 4);
                offset += static_cast<std::uint32_t>((tables[index].size() + 3) / 4 * 4);
            }
            for (const Bytes& table : tables)
            {
                out.insert(out.end(), table.begin(), table.end());
                out.resize((out.size() + 3) / 4 * 4, 0);
            }
            return out;
        }
    }

    // loca — смещения начала каждого глифа и конец последнего / loca — the start offset of every glyph and the end of the last one
    Bytes buildFont(const std::vector<Bytes>& glyphs)
    {
        std::vector<Bytes> tables = fixedTables(glyphs.size());
        Bytes loca;
        Bytes glyf;

        for (const Bytes& glyph : glyphs)
        {
            put(loca, static_cast<std::uint32_t>(glyf.size()), 4);
            glyf.insert(glyf.end(), glyph.begin(), glyph.end());
        }
        put(loca, static_cast<std::uint32_t>(glyf.size()), 4);
        tables.push_back(loca);
        tables.push_back(glyf);
        return assemble(tables);
    }

    // Точки (0,0) (100,0) (100,100) (0,100): дельты X и Y — int16, все на кривой
    // The points (0,0) (100,0) (100,100) (0,100): the X and Y deltas are int16, all on the curve
    Bytes squareGlyph()
    {
        Bytes glyph;

        put(glyph, 1, 2);
        put(glyph, 0, 8);
        put(glyph, 3, 2);
        put(glyph, 0, 2);
        for (int point = 0; point < 4; ++point)
            glyph.push_back(0x01);
        for (std::uint32_t delta : {0u, 100u, 0u, 0xFF9Cu})
            put(glyph, delta, 2);
        for (std::uint32_t delta : {0u, 0u, 100u, 0u})
            put(glyph, delta, 2);
        return glyph;
    }

    Bytes compositeHeader()
    {
        Bytes glyph;

        put(glyph, 0xFFFFu, 2);
        put(glyph, 0, 8);
        return glyph;
    }

    void appendComponent(Bytes& glyph, std::uint16_t flags, std::uint16_t glyphIndex)
    {
        put(glyph, flags, 2);
        put(glyph, glyphIndex, 2);
        if ((flags & ARGS_ARE_WORDS) != 0)
            put(glyph, 0, 4);
        else
            put(glyph, 0, 2);
    }

    // Каталог temp ОС: тесты не пишут в дерево проекта / The OS temp directory: the tests do not write into the project tree
    std::string writeTemp(const Bytes& bytes, const std::string& name)
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() / ("gran_test_" + name);
        std::ofstream file(path, std::ios::binary | std::ios::trunc);

        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return path.string();
    }
}
