#include "gran/text/internal/font_parts.hpp"

#include <algorithm>

namespace gran::text
{
    namespace
    {
        // Предел числа подтаблиц: защита от раздутого испорченного шрифта / The subtable count limit: a guard against a bloated damaged font
        constexpr std::size_t MAX_SUBTABLES = 256;

        // Подтаблицы одного поиска; тип 9 (расширение) раскрывается в настоящий тип и смещение
        // The subtables of one lookup; type 9 (an extension) is unfolded into the real type and offset
        void lookupSubtables(const FontReader& reader, std::size_t lookup, std::vector<std::size_t>& subtables,
            bool& damaged)
        {
            std::uint16_t type = reader.u16(lookup);
            std::uint16_t count = reader.u16(lookup + 4);
            if (!reader.contains(lookup, 6) || !reader.contains(lookup + 6, count * 2u))
            {
                damaged = true;
                return;
            }

            for (std::uint16_t index = 0; index < count && subtables.size() < MAX_SUBTABLES; ++index)
            {
                std::size_t subtable = lookup + reader.u16(lookup + 6 + static_cast<std::size_t>(index) * 2);
                std::uint16_t real = type;
                if (type == 9)
                {
                    if (!reader.contains(subtable, 8) || reader.u16(subtable) != 1)
                    {
                        damaged = true;
                        continue;
                    }
                    real = reader.u16(subtable + 2);
                    const auto relative = reader.u32(subtable + 4);
                    if (relative < 8 || !reader.contains(subtable, relative))
                    {
                        damaged = true;
                        continue;
                    }
                    subtable += relative;
                }
                bool known = std::find(subtables.begin(), subtables.end(), subtable) != subtables.end();
                if (real == 2 && !reader.contains(subtable, 10))
                    damaged = true;
                else if (real == 2 && !known)
                    subtables.push_back(subtable);
            }
        }

        // Поиски одного признака kern / The lookups of one kern feature
        void featureLookups(const FontReader& reader, std::size_t feature, std::size_t lookupList,
            std::vector<std::size_t>& subtables, bool& damaged)
        {
            std::uint16_t count = reader.u16(feature + 2);
            if (!reader.contains(feature, 4) || !reader.contains(feature + 4, count * 2u))
            {
                damaged = true;
                return;
            }

            for (std::uint16_t index = 0; index < count; ++index)
            {
                std::uint16_t lookupIndex = reader.u16(feature + 4 + static_cast<std::size_t>(index) * 2);
                if (lookupIndex >= reader.u16(lookupList))
                {
                    damaged = true;
                    continue;
                }
                lookupSubtables(reader, lookupList + reader.u16(lookupList + 2 + static_cast<std::size_t>(lookupIndex) * 2),
                    subtables, damaged);
            }
        }
    }

    /*
    ** Все признаки 'kern' во всех письменностях: их поиски типа 2 (PairPos). Результат
    ** зависит только от шрифта, поэтому считается один раз при загрузке.
    **
    ** All 'kern' features in all scripts: their type 2 lookups (PairPos). The result
    ** depends only on the font, so it is computed once at load time.
    */
    void FaceBuilder::collectPairSubtables(FontFace& face)
    {
        const TableSpan& gpos = face._tables.gpos;

        if (gpos.length < 10)
            return;
        FontReader reader(face._file->data() + gpos.offset, gpos.length);
        std::size_t features = reader.u16(6);
        std::size_t lookups = reader.u16(8);
        std::uint16_t count = reader.u16(features);
        if (features < 10 || lookups < 10 || !reader.contains(features, 2) || !reader.contains(lookups, 2) ||
            !reader.contains(features + 2, count * 6u) ||
            !reader.contains(lookups + 2, reader.u16(lookups) * 2u))
            return;
        for (std::uint16_t index = 0; index < count; ++index)
        {
            std::size_t record = features + 2 + static_cast<std::size_t>(index) * 6;
            if (reader.u32(record) == fontTag("kern"))
                featureLookups(reader, features + reader.u16(record + 4), lookups, face._pairSubtables,
                    face._damagedPairLookup);
        }
        for (auto& offset : face._pairSubtables)
            offset += gpos.offset;
    }
}
