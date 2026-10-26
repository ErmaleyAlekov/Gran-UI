#ifndef GRAN_RENDER_GRADIENT_RAMPS_HPP
#define GRAN_RENDER_GRADIENT_RAMPS_HPP

#include "gran/style/values.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace gran::render
{
    /*
    ** Рампы градиентов: каждая строка текстуры RGBA8 (256 текселей) — один набор опорных точек
    ** с предумноженной альфой (CSS смешивает цвета градиента именно так). Одинаковые наборы
    ** делят строку. Нет места — рост вдвое (новое поколение, текстура пересоздаётся).
    **
    ** Gradient ramps: each row of an RGBA8 texture (256 texels) is one stop set
    ** with premultiplied alpha (CSS blends gradient colors exactly this way). Equal sets
    ** share a row. No room — doubling (a new generation, the texture is recreated).
    */
    class GradientRamps
    {
    public:
        // ---- gradient_ramps.cpp ----
        explicit GradientRamps(std::uint32_t rows = 32);

        // Строка набора опорных точек градиента / The row of the gradient stop set
        std::uint32_t rowFor(const style::Gradient& gradient);

        // Изменённые строки с прошлого вызова; false — изменений нет / The rows changed since the last call; false — no changes
        bool takeDirty(std::uint32_t& firstRow, std::uint32_t& rowCount);

        // Ёмкость в строках, пиксели, поколение / The capacity in rows, the pixels, the generation
        std::uint32_t capacity() const noexcept;
        const std::uint8_t* pixels() const noexcept;

        // ---- ramp_fill.cpp ----
        std::uint32_t generation() const noexcept;

    private:
        // ---- ramp_fill.cpp ----
        // Заполнение строки интерполяцией опорных точек / Filling a row by interpolating the stops
        void fillRow(std::uint32_t row, const style::Gradient& gradient);

        // Рост вдвое (или сброс за пределом) / Doubling (or a reset beyond the limit)
        void growOrReset();

        std::unordered_map<std::string, std::uint32_t> _rows;
        std::vector<std::uint8_t> _pixels;
        std::uint32_t _capacity;
        std::uint32_t _used = 0;
        std::uint32_t _generation = 1;
        std::uint32_t _dirtyFirst;
        std::uint32_t _dirtyLast = 0;
    };
}

#endif
