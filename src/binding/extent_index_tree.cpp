#include "gran/binding/internal/extent_index.hpp"

namespace gran::binding
{
    /*
    ** Линейная постройка дерева Фенвика: каждый узел сначала получает своё значение, затем
    ** передаёт накопленное родителю (position + младший бит) — O(n) вместо O(n log n).
    **
    ** The linear Fenwick tree construction: every node first gets its own value, then
    ** passes the accumulated value to its parent (position + the lowest bit) — O(n) instead of O(n log n).
    */
    void ExtentIndex::build() const
    {
        std::size_t count = _sizes.size();
        if (!_stale)
            return;
        _treeSum.assign(count + 1, 0.0);
        _treeKnown.assign(count + 1, 0);
        for (std::size_t position = 1; position <= count; ++position)
        {
            float size = _sizes[position - 1];
            if (size >= 0.0f)
            {
                _treeSum[position] += static_cast<double>(size);
                _treeKnown[position] += 1;
            }
            std::size_t parent = position + (position & (~position + 1));
            if (parent <= count)
            {
                _treeSum[parent] += _treeSum[position];
                _treeKnown[parent] += _treeKnown[position];
            }
        }
        _stale = false;
    }

    /*
    ** Подъём по дереву: каждый следующий узел покрывает отрезок, содержащий position. Уменьшение
    ** приходит только от forget измеренной записи — значит, в каждом узле на пути она учтена и
    ** счётчик не меньше единицы.
    **
    ** Climbing the tree: every next node covers a range containing position. A decrease
    ** comes only from forget of a measured record — so every node on the way has it counted and
    ** the counter is at least one.
    */
    void ExtentIndex::add(std::size_t position, double sum, int known)
    {
        for (; position < _treeSum.size(); position += position & (~position + 1))
        {
            _treeSum[position] += sum;
            if (known > 0)
                _treeKnown[position] += 1;
            else if (known < 0)
                _treeKnown[position] -= 1;
        }
    }

    // Спуск по дереву: отрезки (position - младший бит, position] складываются в префикс / Descending the tree: the ranges (position - lowest bit, position] add up to the prefix
    ExtentIndex::Prefix ExtentIndex::prefix(std::size_t count) const
    {
        Prefix result;
        build();
        if (count > _sizes.size())
            count = _sizes.size();
        for (std::size_t position = count; position > 0; position -= position & (~position + 1))
        {
            result.sum += _treeSum[position];
            result.known += _treeKnown[position];
        }
        return result;
    }
}
