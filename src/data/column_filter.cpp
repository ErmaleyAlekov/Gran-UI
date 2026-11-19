#include "gran/data/column_filter.hpp"

#include <utility>

namespace gran::data
{
    ColumnFilter::ColumnFilter(std::shared_ptr<RecordFilter> records, std::shared_ptr<NodeFilter> nodes)
        : _records(std::move(records)), _nodes(std::move(nodes))
    {
    }

    // Условия разбираются один раз здесь, а не на каждую запись / The conditions are parsed once here rather than per record
    void ColumnFilter::setConditions(const std::vector<ColumnCondition>& conditions)
    {
        _conditions.clear();
        for (const ColumnCondition& condition : conditions)
        {
            if (!condition.field.empty() && condition.text.find_first_not_of(" \t") != std::string::npos)
                _conditions.push_back(parseCondition(condition));
        }
    }

    bool ColumnFilter::empty() const noexcept
    {
        return _conditions.empty();
    }

    void ColumnFilter::setBase(std::shared_ptr<RecordFilter> records, std::shared_ptr<NodeFilter> nodes)
    {
        _records = std::move(records);
        _nodes = std::move(nodes);
    }
}
