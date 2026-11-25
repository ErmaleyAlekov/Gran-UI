#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    // Глифы после numberOfHMetrics берут продвижение последней записи (моноширинный хвост)
    // Glyphs after numberOfHMetrics take the advance of the last record (a monospaced tail)
    float FontFace::advance(std::uint16_t glyph) const noexcept
    {
        FontReader reader(_file->data(), _file->size());
        std::uint16_t index = glyph;

        if (index >= _horizontalMetrics)
            index = static_cast<std::uint16_t>(_horizontalMetrics - 1);
        return reader.u16(_tables.hmtx.offset + static_cast<std::size_t>(index) * 4);
    }

    // GPOS точнее и новее; таблица kern — для старых шрифтов без GPOS-кернинга
    // GPOS is more precise and newer; the kern table is for old fonts without GPOS kerning
    float FontFace::kerning(std::uint16_t left, std::uint16_t right) const noexcept
    {
        FontReader reader(_file->data(), _file->size());
        float value = 0.0f;
        FontReader gpos(_file->data() + _tables.gpos.offset, _tables.gpos.length);
        bool useKern = _pairSubtables.empty() || _damagedPairLookup;

        for (std::size_t subtable : _pairSubtables)
        {
            const auto result = pairAdjustment(gpos, subtable - _tables.gpos.offset, left, right, value);
            if (result == PairResult::MATCH)
                return value;
            if (result == PairResult::DAMAGED)
                useKern = true;
        }
        if (useKern && _tables.kern.length > 0)
            return kernTableValue(reader, _tables.kern, left, right);
        return 0.0f;
    }

    // Число глифов / The glyph count
    std::uint16_t FontFace::glyphCount() const noexcept
    {
        return _glyphCount;
    }
}
