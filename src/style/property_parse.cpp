#include "gran/style/property.hpp"
#include "gran/style/internal/gradient_support.hpp"
#include "gran/style/internal/keyword_tables.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/style/value_scanner.hpp"

#include <cmath>

namespace gran::style
{
    using internal::throwValueError;

    namespace
    {
        // Целое без дробной части / An integer without a fractional part
        int parseInteger(std::string_view text)
        {
            float value = parseNumber(text);

            if (std::floor(value) != value)
                throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
            return static_cast<int>(value);
        }

        // normal = 400, bold = 700, иначе число 1..1000 / normal = 400, bold = 700, otherwise a number 1..1000
        int parseFontWeight(std::string_view text)
        {
            static const Keyword NAMES[] = {{"normal", 400}, {"bold", 700}};
            std::string_view trimmed = trimSpaces(text);

            if (!trimmed.empty() && trimmed[0] >= 'a' && trimmed[0] <= 'z')
                return parseKeyword(trimmed, NAMES, 2);
            int weight = parseInteger(trimmed);
            if (weight < 1 || weight > 1000)
                throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
            return weight;
        }

        // Простые виды: число, целое, логическое, ключевое слово, строка, длина
        // The simple kinds: a number, an integer, a boolean, a keyword, a string, a length
        PropertyValue parseScalar(const PropertyInfo& info, std::string_view text)
        {
            std::size_t count = 0;
            const Keyword* words = nullptr;

            switch (info.kind)
            {
                case ValueKind::NUMBER: return parseNumber(text);
                case ValueKind::INTEGER: return parseInteger(text);
                case ValueKind::BOOLEAN: return parseBoolean(text);
                case ValueKind::FONT_WEIGHT: return parseFontWeight(text);
                case ValueKind::STRING: return std::string(trimSpaces(text));
                case ValueKind::LENGTH: return parseLength(text);
                default: break;
            }
            words = internal::keywordTable(info.id, count);
            return parseKeyword(text, words, count);
        }

        // Составные виды / The composite kinds
        PropertyValue parseComposite(const PropertyInfo& info, std::string_view text)
        {
            switch (info.kind)
            {
                case ValueKind::INSETS: return parseInsets(text);
                case ValueKind::RADII: return parseRadii(text);
                case ValueKind::COLOR: return parseColor(text);
                case ValueKind::PAINT: return parsePaint(text);
                case ValueKind::SHADOWS: return parseShadows(text);
                case ValueKind::TRANSFORM: return parseTransform(text);
                case ValueKind::ORIGIN: return internal::parseGradientCenter(text, text);
                case ValueKind::TRANSITIONS: return parseTransitions(text);
                default: return parseTracks(text);
            }
        }
    }

    // Вид значения выбирает парсер / The value kind chooses the parser
    PropertyValue parsePropertyValue(const PropertyInfo& info, std::string_view text)
    {
        switch (info.kind)
        {
            case ValueKind::INSETS: case ValueKind::RADII: case ValueKind::COLOR: case ValueKind::PAINT:
            case ValueKind::SHADOWS: case ValueKind::TRANSFORM: case ValueKind::ORIGIN:
            case ValueKind::TRANSITIONS: case ValueKind::TRACKS:
                return parseComposite(info, text);
            default:
                return parseScalar(info, text);
        }
    }
}
