#include "gran/binding/internal/extent_index.hpp"

#include <algorithm>

namespace gran::binding
{
    namespace
    {
        // Метка неизвестного размера / The unknown size mark
        const float UNKNOWN = -1.0f;
    }

    std::size_t ExtentIndex::size() const noexcept
    {
        return _sizes.size();
    }

    // Номер за концом вставляет в конец; дерево перестроится при следующем запросе / An index past the end inserts at the end; the tree is rebuilt on the next query
    void ExtentIndex::insert(std::size_t index, std::size_t count)
    {
        std::size_t place = std::min(index, _sizes.size());
        if (count == 0)
            return;
        _sizes.insert(_sizes.begin() + static_cast<std::ptrdiff_t>(place), count, UNKNOWN);
        _stale = true;
    }

    /*
    ** Границы считаются без переполнения: count может быть огромным (SIZE_MAX — «до конца»),
    ** поэтому сравнивается с остатком size - index, а не index + count с size.
    **
    ** The bounds are computed without overflow: count may be huge (SIZE_MAX — "to the end"),
    ** so it is compared with the remainder size - index, not index + count with size.
    */
    void ExtentIndex::erase(std::size_t index, std::size_t count)
    {
        if (index >= _sizes.size() || count == 0)
            return;
        std::size_t take = std::min(count, _sizes.size() - index);
        auto from = _sizes.begin() + static_cast<std::ptrdiff_t>(index);
        _sizes.erase(from, from + static_cast<std::ptrdiff_t>(take));
        _stale = true;
    }

    // Новое место за пределами списка — тоже признак чужого отображения: размеры сбрасываются / A new place outside the list is also a sign of a foreign mapping: the sizes are reset
    void ExtentIndex::permute(const std::vector<std::size_t>& oldToNew)
    {
        std::vector<float> moved(_sizes.size(), UNKNOWN);
        _stale = true;
        if (oldToNew.size() != _sizes.size())
        {
            std::fill(_sizes.begin(), _sizes.end(), UNKNOWN);
            return;
        }
        for (std::size_t old = 0; old < _sizes.size(); ++old)
        {
            if (oldToNew[old] >= moved.size())
            {
                std::fill(_sizes.begin(), _sizes.end(), UNKNOWN);
                return;
            }
            moved[oldToNew[old]] = _sizes[old];
        }
        _sizes = std::move(moved);
    }
}
