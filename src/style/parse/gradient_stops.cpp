#include "gran/style/internal/gradient_support.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_parsers.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style::internal
{
    namespace
    {
        // Сообщение о числе точек / The stop count message
        const char* const TWO_STOPS = "≥ 2 stops";

        // Одна запись "цвет [поз [поз]]" даёт одну или две точки
        // One "color [pos [pos]]" entry gives one or two stops
        void parseStop(std::string_view entry, std::vector<GradientStop>& stops, std::string_view text)
        {
            std::vector<std::string_view> parts = splitSpaces(entry);

            if (parts.empty() || parts.size() > 3)
                throwValueError(i18n::Message::STYLE_BAD_GRADIENT, text, entry);
            GradientStop stop{parseColor(parts[0]), -1.0f};
            if (parts.size() >= 2)
                stop.position = parseFraction(parts[1]);
            stops.push_back(stop);
            if (parts.size() == 3)
            {
                stop.position = parseFraction(parts[2]);
                stops.push_back(stop);
            }
        }

        // Позиции без значения: равномерно между известными соседями
        // Positions without a value: evenly between the known neighbours
        void spreadMissing(std::vector<GradientStop>& stops)
        {
            std::size_t known = 0;

            for (std::size_t index = 1; index < stops.size(); ++index)
            {
                if (stops[index].position < 0.0f)
                    continue;
                float from = stops[known].position;
                float step = (stops[index].position - from) / static_cast<float>(index - known);
                for (std::size_t middle = known + 1; middle < index; ++middle)
                    stops[middle].position = from + step * static_cast<float>(middle - known);
                known = index;
            }
        }
    }

    // Края по умолчанию 0 и 1, затем равномерная расстановка и неубывание
    // The edges default to 0 and 1, then even spreading and non-decrease
    std::vector<GradientStop> parseStops(const std::vector<std::string_view>& arguments,
        std::size_t first, std::string_view text)
    {
        std::vector<GradientStop> stops;

        for (std::size_t index = first; index < arguments.size(); ++index)
            parseStop(arguments[index], stops, text);
        if (stops.size() < 2)
            throwValueError(i18n::Message::STYLE_BAD_GRADIENT, text, TWO_STOPS);
        if (stops.front().position < 0.0f)
            stops.front().position = 0.0f;
        if (stops.back().position < 0.0f)
            stops.back().position = 1.0f;
        spreadMissing(stops);
        for (std::size_t index = 1; index < stops.size(); ++index)
        {
            if (stops[index].position < stops[index - 1].position)
                stops[index].position = stops[index - 1].position;
        }
        return stops;
    }

    // Ключевые слова переводятся в доли, остальное — проценты или доли
    // Keywords are turned into fractions, the rest is percent or fractions
    Vec2 parseGradientCenter(std::string_view text, std::string_view original)
    {
        static const Keyword POSITIONS[] = {
            {"left", 0}, {"top", 0}, {"center", 50}, {"right", 100}, {"bottom", 100}};
        std::vector<std::string_view> parts = splitSpaces(text);
        float values[2] = {0.5f, 0.5f};

        if (parts.empty() || parts.size() > 2)
            throwValueError(i18n::Message::STYLE_BAD_GRADIENT, original, text);
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            if (!parts[index].empty() && (parts[index][0] < 'a' || parts[index][0] > 'z'))
                values[index] = parseFraction(parts[index]);
            else
                values[index] = parseKeyword(parts[index], POSITIONS) / 100.0f;
        }
        return Vec2{values[0], values[1]};
    }
}
