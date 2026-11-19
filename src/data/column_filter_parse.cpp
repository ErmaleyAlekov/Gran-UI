#include "gran/data/column_filter.hpp"
#include "gran/data/internal/cell_formats.hpp"

#include <string_view>

namespace gran::data
{
    namespace
    {
        // Текст без пробелов и табуляций по краям / The text without spaces and tabs at the edges
        std::string_view trimmed(std::string_view text) noexcept
        {
            std::size_t first = text.find_first_not_of(" \t");
            std::size_t last = text.find_last_not_of(" \t");

            if (first == std::string_view::npos)
                return std::string_view();
            return text.substr(first, last - first + 1);
        }

        /*
        ** Оператор сравнения в начале текста: двухсимвольные проверяются раньше односимвольных («<=» — не «<» и «=5»).
        ** length — длина оператора; 0 — оператора нет.
        **
        ** A comparison operator at the text start: the two-character ones are checked before the one-character ones ("<=" is not "<" and "=5").
        ** length is the operator length; 0 — no operator.
        */
        FilterOperator operatorOf(std::string_view text, std::size_t& length) noexcept
        {
            static const struct { const char* sign; FilterOperator op; } SIGNS[] = {
                {"<=", FilterOperator::LESS_EQUAL}, {">=", FilterOperator::GREATER_EQUAL}, {"<>", FilterOperator::NOT_EQUAL},
                {"<", FilterOperator::LESS}, {">", FilterOperator::GREATER}, {"=", FilterOperator::EQUAL}};

            for (const auto& entry : SIGNS)
            {
                std::string_view sign(entry.sign);
                if (text.substr(0, sign.size()) == sign)
                {
                    length = sign.size();
                    return entry.op;
                }
            }
            length = 0;
            return FilterOperator::CONTAINS;
        }

        // Итог сравнения чисел (-1, 0, 1) под оператор / The number comparison result (-1, 0, 1) against the operator
        bool comparisonHolds(FilterOperator op, int order) noexcept
        {
            if (op == FilterOperator::LESS)
                return order < 0;
            if (op == FilterOperator::LESS_EQUAL)
                return order <= 0;
            if (op == FilterOperator::GREATER)
                return order > 0;
            if (op == FilterOperator::GREATER_EQUAL)
                return order >= 0;
            if (op == FilterOperator::EQUAL)
                return order == 0;
            return order != 0;
        }
    }

    ParsedCondition parseCondition(const ColumnCondition& condition)
    {
        ParsedCondition parsed;
        std::string_view text = trimmed(condition.text);
        std::size_t length = 0;
        FilterOperator op = operatorOf(text, length);

        parsed.field = condition.field;
        if (condition.numeric && length > 0
            && internal::normalizeNumber(trimmed(text.substr(length)), parsed.operand))
        {
            parsed.op = op;
            return parsed;
        }
        parsed.op = FilterOperator::CONTAINS;
        parsed.operand = foldCase(text);
        return parsed;
    }

    // Сравнение чисел: значение, которое не число, условию не подходит (пустая ячейка не «меньше нуля») / A number comparison: a value that is not a number does not match (an empty cell is not "less than zero")
    bool conditionMatches(const ParsedCondition& condition, const std::string& value)
    {
        std::string number;

        if (condition.op == FilterOperator::CONTAINS)
            return foldCase(value).find(condition.operand) != std::string::npos;
        if (!internal::normalizeNumber(trimmed(value), number))
            return false;
        return comparisonHolds(condition.op, compareNumber(number, condition.operand));
    }
}
