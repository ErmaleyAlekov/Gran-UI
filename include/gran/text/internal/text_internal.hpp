#ifndef GRAN_TEXT_INTERNAL_TEXT_INTERNAL_HPP
#define GRAN_TEXT_INTERNAL_TEXT_INTERNAL_HPP

#include "gran/text/text_layout.hpp"

namespace gran::text
{
    // ---- break_rules.cpp ----

    /*
    ** Возможность переноса после символа (упрощённый UAX #14): после пробела, табуляции и
    ** пробела нулевой ширины, после дефиса перед буквой, до и после иероглифа CJK.
    **
    ** A break opportunity after the character (a simplified UAX #14): after a space, a tab and
    ** a zero width space, after a hyphen before a letter, before and after a CJK ideograph.
    */
    bool breakOpportunityAfter(char32_t symbol, char32_t next) noexcept;

    // Пробельный символ для переноса и выравнивания по ширине / A whitespace character for breaking and justification
    bool isBreakingSpace(char32_t symbol) noexcept;

    // ---- line_metrics.cpp ----

    // Ширина глифов [first, last) без висячих пробелов в конце / The width of the glyphs [first, last) without trailing spaces
    float lineWidth(const std::vector<ShapedGlyph>& glyphs, std::size_t first, std::size_t last) noexcept;

    // ---- text_layout.cpp ----

    // Сформированные глифы → строки и места (перенос, многоточие, интервал, выравнивание); основное начертание — faces[0]
    // The shaped glyphs → lines and places (breaking, the ellipsis, spacing, alignment); the primary face is faces[0]
    void placeShaped(std::vector<ShapedGlyph>& glyphs, const TextStyle& style, float maxWidth, TextLayout& layout);
}

#endif
