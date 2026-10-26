#include "gran/style/internal/keyword_tables.hpp"
#include "gran/style/computed_style.hpp"

namespace gran::style::internal
{
    namespace
    {
        // Перевод перечисления в значение таблицы / An enumeration to a table value
        template <typename Enum>
        constexpr int v(Enum value)
        {
            return static_cast<int>(value);
        }

        const Keyword ORIENTATIONS[] = {{"horizontal", v(Orientation::HORIZONTAL)}, {"vertical", v(Orientation::VERTICAL)}};
        const Keyword JUSTIFICATIONS[] = {{"start", v(Justify::START)}, {"center", v(Justify::CENTER)},
            {"end", v(Justify::END)}, {"space-between", v(Justify::SPACE_BETWEEN)},
            {"space-around", v(Justify::SPACE_AROUND)}, {"space-evenly", v(Justify::SPACE_EVENLY)}};
        const Keyword ALIGNMENTS[] = {{"start", v(Align::START)}, {"center", v(Align::CENTER)},
            {"end", v(Align::END)}, {"stretch", v(Align::STRETCH)}, {"auto", v(Align::AUTO)}};
        const Keyword TEXT_ALIGNMENTS[] = {{"start", v(TextAlign::START)}, {"center", v(TextAlign::CENTER)},
            {"end", v(TextAlign::END)}, {"justify", v(TextAlign::JUSTIFY)}};
        const Keyword OVERFLOWS[] = {{"visible", v(Overflow::VISIBLE)}, {"hidden", v(Overflow::HIDDEN)}};
        const Keyword CURSORS[] = {{"default", v(Cursor::DEFAULT)}, {"pointer", v(Cursor::POINTER)},
            {"text", v(Cursor::TEXT)}, {"move", v(Cursor::MOVE)}, {"not-allowed", v(Cursor::NOT_ALLOWED)},
            {"grab", v(Cursor::GRAB)}, {"ew-resize", v(Cursor::EW_RESIZE)}, {"ns-resize", v(Cursor::NS_RESIZE)}};
        const Keyword FONT_STYLES[] = {{"normal", 0}, {"italic", 1}};
        const Keyword TEXT_WRAPS[] = {{"wrap", v(TextWrap::WRAP)}, {"nowrap", v(TextWrap::NOWRAP)}};
        const Keyword TEXT_OVERFLOWS[] = {{"clip", v(TextOverflow::CLIP)}, {"ellipsis", v(TextOverflow::ELLIPSIS)}};
        const Keyword TEXT_DECORATIONS[] = {{"none", v(TextDecoration::NONE)}, {"underline", v(TextDecoration::UNDERLINE)},
            {"lineThrough", v(TextDecoration::LINE_THROUGH)}};

        // Таблица свойства: идентификатор, слова, их число / A property table: the identifier, the words, their count
        struct KeywordTable
        {
            PropertyId id;
            const Keyword* words;
            std::size_t count;
        };

        const KeywordTable TABLES[] = {
            {PropertyId::ORIENTATION, ORIENTATIONS, 2}, {PropertyId::JUSTIFY, JUSTIFICATIONS, 6},
            {PropertyId::ALIGN, ALIGNMENTS, 4}, {PropertyId::ALIGN_SELF, ALIGNMENTS, 5},
            {PropertyId::TEXT_ALIGN, TEXT_ALIGNMENTS, 4}, {PropertyId::OVERFLOW_MODE, OVERFLOWS, 2},
            {PropertyId::CURSOR, CURSORS, 8}, {PropertyId::FONT_STYLE, FONT_STYLES, 2},
            {PropertyId::TEXT_WRAP, TEXT_WRAPS, 2}, {PropertyId::TEXT_OVERFLOW, TEXT_OVERFLOWS, 2},
            {PropertyId::TEXT_DECORATION, TEXT_DECORATIONS, 3}};
    }

    // Таблица ключевых слов свойства; count = 0 — у свойства их нет
    // The property keyword table; count = 0 — the property has none
    const Keyword* keywordTable(PropertyId id, std::size_t& count) noexcept
    {
        for (const KeywordTable& table : TABLES)
        {
            if (table.id == id)
            {
                count = table.count;
                return table.words;
            }
        }
        count = 0;
        return nullptr;
    }
}
