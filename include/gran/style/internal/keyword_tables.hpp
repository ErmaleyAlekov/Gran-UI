#ifndef GRAN_STYLE_INTERNAL_KEYWORD_TABLES_HPP
#define GRAN_STYLE_INTERNAL_KEYWORD_TABLES_HPP

#include "gran/style/property.hpp"
#include "gran/style/value_parsers.hpp"

namespace gran::style::internal
{
    // Таблица ключевых слов свойства; count = 0 — у свойства их нет
    // The property keyword table; count = 0 — the property has none
    const Keyword* keywordTable(PropertyId id, std::size_t& count) noexcept;
}

#endif
