#include "gran/vector/internal/vector_internal.hpp"

#include <atomic>

namespace gran::vector
{
    // Четыре числа подряд и ничего после них; ширина и высота больше нуля / Four numbers in a row and nothing after them; the width and height are above zero
    bool parseViewBox(std::string_view text, Rect& viewBox)
    {
        internal::PathScanner scanner{text, 0};
        float values[4] = {};

        for (float& value : values)
        {
            if (!internal::readNumber(scanner, value))
                return false;
        }
        internal::skipSeparators(scanner);
        if (scanner.pos != text.size() || values[2] <= 0.0f || values[3] <= 0.0f)
            return false;
        viewBox = Rect{values[0], values[1], values[2], values[3]};
        return true;
    }

    // Атомарный счётчик: фигуры строятся и при загрузке разметки, и рендерером / An atomic counter: shapes are built both on markup loading and by the renderer
    std::uint32_t nextShapeId() noexcept
    {
        static std::atomic<std::uint32_t> counter{0};

        return ++counter;
    }
}
