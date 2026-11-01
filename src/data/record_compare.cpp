#include "gran/data/record_order.hpp"

#include "xpath/xpath_value.hpp"

#include <cmath>

namespace gran::data
{
    namespace
    {
        // Значение поля; отсутствующее — пустая строка / The field value; a missing one is an empty string
        const std::string& fieldOf(const Record& record, const std::string& field)
        {
            static const std::string EMPTY;
            auto found = record.find(field);

            if (found == record.end())
                return EMPTY;
            return found->second;
        }
    }

    // UTF-8 по байтам сравнивается в порядке кодов символов / UTF-8 compared by bytes follows the character code order
    int compareText(std::string_view left, std::string_view right)
    {
        int order = foldCase(left).compare(foldCase(right));

        if (order == 0)
            order = left.compare(right);
        return (order > 0) - (order < 0);
    }

    int compareNumber(std::string_view left, std::string_view right)
    {
        double a = beresta::xpath::stringToNumber(left);
        double b = beresta::xpath::stringToNumber(right);

        if (std::isnan(a) && std::isnan(b))
            return compareText(left, right);
        if (std::isnan(a))
            return 1;
        if (std::isnan(b))
            return -1;
        return (a > b) - (a < b);
    }

    int compareRecords(const Record& left, const Record& right, const SortKeys& keys)
    {
        for (const SortKey& key : keys)
        {
            const std::string& first = fieldOf(left, key.field);
            const std::string& second = fieldOf(right, key.field);
            int order = 0;
            if (key.type == SortType::NUMBER)
                order = compareNumber(first, second);
            else if (key.type == SortType::NATURAL)
                order = compareNatural(first, second);
            else
                order = compareText(first, second);
            if (order != 0 && key.descending)
                return -order;
            if (order != 0)
                return order;
        }
        return 0;
    }
}
