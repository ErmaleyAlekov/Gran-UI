#include "gran/binding/internal/extent_index.hpp"

#include <cmath>

namespace gran::binding
{
    /*
    ** Новое измерение обновляет дерево за O(log n); устаревшее дерево не трогается — оно
    ** всё равно перестроится из _sizes. NaN и отрицательные размеры считаются нулём: размер
    ** приходит из компоновки, и испорченное значение не должно отравить все суммы.
    **
    ** A new measurement updates the tree in O(log n); a stale tree is not touched — it will
    ** be rebuilt from _sizes anyway. NaN and negative sizes count as zero: the size
    ** comes from the layout, and a broken value must not poison all the sums.
    */
    void ExtentIndex::set(std::size_t index, float extent)
    {
        if (index >= _sizes.size())
            return;
        float value = 0.0f;
        if (extent > 0.0f && std::isfinite(extent))
            value = extent;
        float old = _sizes[index];
        bool known = old >= 0.0f;
        _sizes[index] = value;
        if (_stale)
            return;
        if (known)
            add(index + 1, static_cast<double>(value) - static_cast<double>(old), 0);
        else
            add(index + 1, static_cast<double>(value), 1);
    }

    void ExtentIndex::forget(std::size_t index)
    {
        if (index >= _sizes.size() || _sizes[index] < 0.0f)
            return;
        float old = _sizes[index];
        _sizes[index] = -1.0f;
        if (!_stale)
            add(index + 1, -static_cast<double>(old), -1);
    }

    float ExtentIndex::sizeAt(std::size_t index) const
    {
        if (index < _sizes.size() && _sizes[index] >= 0.0f)
            return _sizes[index];
        return estimate();
    }

    float ExtentIndex::estimate() const
    {
        Prefix all = prefix(_sizes.size());
        if (all.known == 0)
            return 0.0f;
        return static_cast<float>(all.sum / static_cast<double>(all.known));
    }
}
