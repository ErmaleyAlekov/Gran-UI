#include "gran/text/internal/font_parts.hpp"

namespace gran::text
{
    /*
    ** Глиф из loca: пустой (длина 0, как у пробела) или короче заголовка — false;
    ** конец глифа не выходит за таблицу glyf (её границы проверены при загрузке).
    **
    ** A glyph from loca: an empty one (length 0, as for a space) or shorter than the header — false;
    ** the glyph end does not go beyond the glyf table (its bounds are checked at load time).
    */
    bool glyphLocation(const FontReader& reader, const FaceTables& tables, bool longLoca, std::uint16_t glyph,
        std::uint32_t& offset, std::uint32_t& end) noexcept
    {
        std::uint32_t first = 0;
        std::uint32_t last = 0;
        std::size_t entry = static_cast<std::size_t>(glyph);

        if (longLoca)
        {
            first = reader.u32(tables.loca.offset + entry * 4);
            last = reader.u32(tables.loca.offset + entry * 4 + 4);
        }
        else
        {
            first = static_cast<std::uint32_t>(reader.u16(tables.loca.offset + entry * 2)) * 2;
            last = static_cast<std::uint32_t>(reader.u16(tables.loca.offset + entry * 2 + 2)) * 2;
        }
        if (last <= first || last > tables.glyf.length || last - first < 10)
            return false;
        offset = tables.glyf.offset + first;
        end = tables.glyf.offset + last;
        return true;
    }
}
