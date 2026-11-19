#include "gran/data/column_filter.hpp"

namespace gran::data
{
    const std::shared_ptr<RecordFilter>& ColumnFilter::baseRecords() const noexcept
    {
        return _records;
    }

    const std::shared_ptr<NodeFilter>& ColumnFilter::baseNodes() const noexcept
    {
        return _nodes;
    }

    // Сначала свои условия (дёшево), потом прежний фильтр (выражение XPath — дороже) / Own conditions first (cheap), then the former filter (an XPath expression is dearer)
    bool ColumnFilter::accepts(const ListSource& source, std::size_t index) const
    {
        if (!matches(source.at(index)))
            return false;
        return _records == nullptr || _records->accepts(source, index);
    }

    bool ColumnFilter::accepts(const TreeModel& tree, NodeId node) const
    {
        if (!matches(tree.record(node)))
            return false;
        return _nodes == nullptr || _nodes->accepts(tree, node);
    }

    // Нет поля в записи — значение пустое (как пустая ячейка) / No field in the record — the value is empty (as an empty cell)
    bool ColumnFilter::matches(const Record& record) const
    {
        static const std::string EMPTY;

        for (const ParsedCondition& condition : _conditions)
        {
            Record::const_iterator found = record.find(condition.field);
            const std::string* value = &EMPTY;
            if (found != record.end())
                value = &found->second;
            if (!conditionMatches(condition, *value))
                return false;
        }
        return true;
    }
}
