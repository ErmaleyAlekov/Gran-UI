#include "gran/render/gradient_ramps.hpp"
#include "gran/render/gpu_types.hpp"

#include <algorithm>
#include <cstdio>

namespace gran::render
{
    namespace
    {
        // Ключ набора: позиции и цвета точек (с точностью до 1/1000) / The set key: the stop positions and colors (to 1/1000)
        std::string rampKey(const style::Gradient& gradient)
        {
            std::string key;
            char part[96];

            for (const style::GradientStop& stop : gradient.stops)
            {
                std::snprintf(part, sizeof(part), "%.3f:%.3f,%.3f,%.3f,%.3f;", stop.position, stop.color.r,
                    stop.color.g, stop.color.b, stop.color.a);
                key += part;
            }
            return key;
        }
    }

    // Пустые строки; грязный диапазон пуст / Empty rows; the dirty range is empty
    GradientRamps::GradientRamps(std::uint32_t rows)
        : _pixels(static_cast<std::size_t>(rows) * RAMP_WIDTH * 4, 0), _capacity(rows), _dirtyFirst(rows)
    {
    }

    // Известный набор — его строка; новый — следующая свободная (при нехватке — рост)
    // A known set — its row; a new one — the next free row (growing when short)
    std::uint32_t GradientRamps::rowFor(const style::Gradient& gradient)
    {
        std::string key = rampKey(gradient);
        auto found = _rows.find(key);

        if (found != _rows.end())
            return found->second;
        if (_used == _capacity)
            growOrReset();
        std::uint32_t row = _used++;
        fillRow(row, gradient);
        _rows.emplace(key, row);
        _dirtyFirst = std::min(_dirtyFirst, row);
        _dirtyLast = std::max(_dirtyLast, row + 1);
        return row;
    }

    // Диапазон изменённых строк и его сброс / The range of the changed rows and clearing it
    bool GradientRamps::takeDirty(std::uint32_t& firstRow, std::uint32_t& rowCount)
    {
        if (_dirtyLast <= _dirtyFirst)
            return false;
        firstRow = _dirtyFirst;
        rowCount = _dirtyLast - _dirtyFirst;
        _dirtyFirst = _capacity;
        _dirtyLast = 0;
        return true;
    }

    // Ёмкость в строках / The capacity in rows
    std::uint32_t GradientRamps::capacity() const noexcept
    {
        return _capacity;
    }

    // Пиксели всех строк / The pixels of all rows
    const std::uint8_t* GradientRamps::pixels() const noexcept
    {
        return _pixels.data();
    }
}
