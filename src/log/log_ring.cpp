#include "gran/log/internal/log_state.hpp"

#include <algorithm>

namespace gran::log
{
    namespace internal
    {
        // Байты идут по кругу; после первого оборота кольцо считается полным / The bytes go around; after the first turn the ring is full
        void appendRing(LogState& state, std::string_view line) noexcept
        {
            auto cursor = state.ringCursor.load(std::memory_order_relaxed);
            for (char symbol : line)
            {
                state.ring[cursor % RING_BYTES].store(symbol, std::memory_order_relaxed);
                ++cursor;
                if (cursor == 2 * RING_BYTES)
                    cursor = RING_BYTES;
                state.ringCursor.store(cursor, std::memory_order_release);
            }
        }
    }

    /*
    ** Копия хвоста кольца без замка: позиция и заполненность берутся одним чтением.
    ** Другая нить может переписать часть байтов за время копирования.
    **
    ** A lock-free copy of the tail: read position and fullness together.
    ** Another thread may overwrite some bytes during the copy.
    */
    std::size_t copyRecent(char* buffer, std::size_t size) noexcept
    {
        const internal::LogState& state = internal::logState();
        const auto cursor = state.ringCursor.load(std::memory_order_acquire);
        const auto count = std::min(std::min(cursor, internal::RING_BYTES), size);
        const auto start = (cursor + internal::RING_BYTES - count) % internal::RING_BYTES;
        for (std::size_t index = 0; index < count; ++index)
            buffer[index] = state.ring[(start + index) % internal::RING_BYTES].load(std::memory_order_relaxed);
        return count;
    }
}
