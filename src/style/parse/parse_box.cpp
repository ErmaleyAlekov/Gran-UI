#include "gran/style/value_parsers.hpp"
#include "gran/style/internal/value_support.hpp"
#include "gran/style/value_scanner.hpp"

namespace gran::style
{
    using internal::throwValueError;

    namespace
    {
        // Сообщение о числе значений / The value count message
        const char* const ONE_TO_FOUR = "1–4";

        // 1–4 числа; иначе ошибка с подсказкой о количестве
        // 1–4 numbers; otherwise an error with a hint about the count
        std::vector<float> fourValues(std::string_view text)
        {
            std::vector<float> values = parseNumberList(text);

            if (values.empty() || values.size() > 4)
                throwValueError(i18n::Message::STYLE_BAD_LIST_SIZE, text, ONE_TO_FOUR);
            return values;
        }

        // Раскрытие 1–4 значений по правилу CSS: [a] → a a a a, [a b] → a b a b, [a b c] → a b c b
        // Expanding 1–4 values by the CSS rule: [a] → a a a a, [a b] → a b a b, [a b c] → a b c b
        void expandFour(std::vector<float>& values)
        {
            if (values.size() == 1)
                values.push_back(values[0]);
            if (values.size() == 2)
                values.push_back(values[0]);
            if (values.size() == 3)
                values.push_back(values[1]);
        }
    }

    // Числа через запятые или пробелы: "8, 16" и "8 16" одинаковы
    // Numbers separated by commas or spaces: "8, 16" and "8 16" are the same
    std::vector<float> parseNumberList(std::string_view text)
    {
        std::vector<float> values;
        ValueScanner scanner(text);
        float value = 0.0f;

        while (!scanner.atEnd())
        {
            if (!scanner.readNumber(value))
                throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
            values.push_back(value);
            scanner.consume(',');
        }
        return values;
    }

    // Отступы в порядке CSS: верх, право, низ, лево
    // Insets in the CSS order: top, right, bottom, left
    Insets parseInsets(std::string_view text)
    {
        std::vector<float> values = fourValues(text);

        expandFour(values);
        return Insets{values[0], values[1], values[2], values[3]};
    }

    // Радиусы: левый верхний, правый верхний, правый нижний, левый нижний; отрицательные запрещены
    // Radii: top left, top right, bottom right, bottom left; negative ones are forbidden
    CornerRadii parseRadii(std::string_view text)
    {
        std::vector<float> values = fourValues(text);

        expandFour(values);
        for (float value : values)
        {
            if (value < 0.0f)
                throwValueError(i18n::Message::STYLE_BAD_NUMBER, text);
        }
        return CornerRadii{values[0], values[1], values[2], values[3]};
    }
}
