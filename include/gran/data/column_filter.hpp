#ifndef GRAN_DATA_COLUMN_FILTER_HPP
#define GRAN_DATA_COLUMN_FILTER_HPP

#include "gran/data/record_order.hpp"
#include "gran/data/tree_list.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran::data
{
    // Сравнение условия столбца: подстрока без учёта регистра или числовое (">100", "<=5", "=0", "<>3")
    // The column condition comparison: a case-insensitive substring or a numeric one (">100", "<=5", "=0", "<>3")
    enum class FilterOperator : std::uint8_t { CONTAINS, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL, NOT_EQUAL };

    /*
    ** Условие столбца, как его набрал пользователь: поле записи, текст и можно ли понимать текст как сравнение чисел
    ** (столбец числовой). Пустой текст — условия нет.
    **
    ** A column condition as the user typed it: the record field, the text and whether the text may mean a number comparison
    ** (the column is numeric). An empty text — no condition.
    */
    struct ColumnCondition
    {
        std::string field;
        std::string text;
        bool numeric = false;
    };

    /*
    ** Разобранное условие: поле, оператор и операнд — свёрнутый регистр текста у CONTAINS, приведённое число у сравнений.
    **
    ** A parsed condition: the field, the operator and the operand — the case-folded text for CONTAINS, the normalized number for comparisons.
    */
    struct ParsedCondition
    {
        std::string field;
        FilterOperator op = FilterOperator::CONTAINS;
        std::string operand;
    };

    // ---- column_filter_parse.cpp ----

    /*
    ** Текст условия → разобранное условие. У числового столбца ">", ">=", "<", "<=", "=", "<>" перед числом — сравнение;
    ** если после оператора не число (или столбец не числовой) — поиск подстроки всего текста.
    **
    ** A condition text → a parsed condition. For a numeric column ">", ">=", "<", "<=", "=", "<>" before a number is a comparison;
    ** if the operator is not followed by a number (or the column is not numeric) — a substring search of the whole text.
    */
    ParsedCondition parseCondition(const ColumnCondition& condition);

    // Значение поля проходит условие / The field value passes the condition
    bool conditionMatches(const ParsedCondition& condition, const std::string& value);

    /*
    ** Фильтр строки фильтров таблицы (паттерн «Декоратор» над прежним фильтром): запись видна, если её пропускает
    ** фильтр разметки (выражение filter у <SortFilter> или <TreeList>; может не быть) и подходит под все условия столбцов.
    ** Годится и для прокси (RecordFilter), и для плоского дерева (NodeFilter — предки подходящих узлов видны сами).
    **
    ** The table filter row filter (the Decorator pattern over the former filter): a record is shown if the markup filter
    ** (the filter expression of <SortFilter> or <TreeList>; may be absent) passes it and it matches all the column conditions.
    ** It suits both a proxy (RecordFilter) and a flat tree (NodeFilter — the ancestors of matching nodes are shown by themselves).
    */
    class ColumnFilter final : public RecordFilter, public NodeFilter
    {
    public:
        // ---- column_filter.cpp ----

        ColumnFilter(std::shared_ptr<RecordFilter> records, std::shared_ptr<NodeFilter> nodes);

        // Условия столбцов (пустые тексты отбрасываются) / The column conditions (the empty texts are dropped)
        void setConditions(const std::vector<ColumnCondition>& conditions);
        bool empty() const noexcept;

        // Прежний фильтр, который декорируется (его заменяет и новое выражение разметки) / The former filter being decorated (a new markup expression replaces it too)
        void setBase(std::shared_ptr<RecordFilter> records, std::shared_ptr<NodeFilter> nodes);

        // ---- column_filter_accept.cpp ----

        const std::shared_ptr<RecordFilter>& baseRecords() const noexcept;
        const std::shared_ptr<NodeFilter>& baseNodes() const noexcept;

        bool accepts(const ListSource& source, std::size_t index) const override;
        bool accepts(const TreeModel& tree, NodeId node) const override;

    private:
        // Запись подходит под все условия / The record matches all the conditions
        bool matches(const Record& record) const;

        std::shared_ptr<RecordFilter> _records;
        std::shared_ptr<NodeFilter> _nodes;
        std::vector<ParsedCondition> _conditions;
    };
}

#endif
