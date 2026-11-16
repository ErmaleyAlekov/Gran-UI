#include "gran/binding/internal/extent_index.hpp"

namespace gran::binding
{
    // Измеренные записи — своими размерами, неизмеренные — оценкой / The measured records count with their sizes, the unmeasured ones with the estimate
    double ExtentIndex::offsetOf(std::size_t index) const
    {
        if (index > _sizes.size())
            index = _sizes.size();
        Prefix before = prefix(index);
        double guess = static_cast<double>(estimate());
        return before.sum + static_cast<double>(index - before.known) * guess;
    }

    /*
    ** Спуск Фенвика по размеру с оценкой: на шаге step узел position + step покрывает ровно
    ** step записей после position, его вес — сумма измеренных плюс (step - измеренных) × оценка.
    ** Узел берётся целиком, пока накопленное не превышает offset; в итоге position — число
    ** записей, закончившихся не позже offset, то есть номер записи, на которую он приходится.
    **
    ** A Fenwick descent by size with the estimate: at step step the node position + step covers exactly
    ** step records after position, its weight is the measured sum plus (step - measured) × the estimate.
    ** A node is taken whole while the accumulated value does not exceed offset; in the end position is the count
    ** of the records that ended no later than offset, that is the index of the record it falls on.
    */
    std::size_t ExtentIndex::indexAt(double offset) const
    {
        std::size_t count = _sizes.size();
        std::size_t position = 0;
        std::size_t step = 1;
        double reached = 0.0;

        // Дерево строится явно: спуск читает _treeSum напрямую, а не через prefix (ревью Т-60)
        // The tree is built explicitly: the descent reads _treeSum directly, not through prefix (review T-60)
        build();
        double guess = static_cast<double>(estimate());
        if (count == 0 || !(offset > 0.0))
            return 0;
        while (step * 2 <= count)
            step *= 2;
        for (; step > 0; step /= 2)
        {
            std::size_t next = position + step;
            if (next > count)
                continue;
            double weight = _treeSum[next] + static_cast<double>(step - _treeKnown[next]) * guess;
            if (reached + weight <= offset)
            {
                position = next;
                reached += weight;
            }
        }
        if (position >= count)
            return count - 1;
        return position;
    }

    double ExtentIndex::total() const
    {
        return offsetOf(_sizes.size());
    }
}
