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
        void lookupSubtables(const FontReader& reader, std::size_t lookup, std::vector<std::uint32_t>& subtables)
        {
            std::uint16_t type = reader.u16(lookup);
            std::uint16_t count = reader.u16(lookup + 4);

            for (std::uint16_t index = 0; index < count && subtables.size() < MAX_SUBTABLES; ++index)
            {
                std::size_t subtable = lookup + reader.u16(lookup + 6 + static_cast<std::size_t>(index) * 2);
                std::uint16_t real = type;
                if (type == 9)
                {
                    real = reader.u16(subtable + 2);
                    subtable += reader.u32(subtable + 4);
                }
                bool known = std::find(subtables.begin(), subtables.end(), subtable) != subtables.end();
                if (real == 2 && !known && reader.contains(subtable, 16))
                    subtables.push_back(static_cast<std::uint32_t>(subtable));
            }
        }

        // Поиски одного признака kern / The lookups of one kern feature
        void featureLookups(const FontReader& reader, std::size_t feature, std::size_t lookupList,
            std::vector<std::uint32_t>& subtables)
        {
            std::uint16_t count = reader.u16(feature + 2);

            for (std::uint16_t index = 0; index < count; ++index)
            {
                std::uint16_t lookupIndex = reader.u16(feature + 4 + static_cast<std::size_t>(index) * 2);
                if (lookupIndex >= reader.u16(lookupList))
                    continue;
                lookupSubtables(reader, lookupList + reader.u16(lookupList + 2 + static_cast<std::size_t>(lookupIndex) * 2),
                    subtables);
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
        FontReader reader(face._file->data(), face._file->size());
        const TableSpan& gpos = face._tables.gpos;

        if (gpos.length < 10)
            return;
        std::size_t features = gpos.offset + reader.u16(gpos.offset + 6);
        std::size_t lookups = gpos.offset + reader.u16(gpos.offset + 8);
        std::uint16_t count = reader.u16(features);
        for (std::uint16_t index = 0; index < count; ++index)
        {
            std::size_t record = features + 2 + static_cast<std::size_t>(index) * 6;
            if (reader.u32(record) == fontTag("kern"))
                featureLookups(reader, features + reader.u16(record + 4), lookups, face._pairSubtables);
        }
    }
}
