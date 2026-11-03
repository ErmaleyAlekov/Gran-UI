#include "gran/log/internal/log_state.hpp"

#include <algorithm>

namespace gran::log
{
    namespace internal
    {
        // Байты идут по кругу; после первого оборота кольцо считается полным / The bytes go around; after the first turn the ring is full
        void appendRing(LogState& state, std::string_view line) noexcept
        {
            for (char symbol : line)
            {
                state.ring[state.ringHead] = symbol;
                state.ringHead = (state.ringHead + 1) % RING_BYTES;
                if (state.ringHead == 0)
                    state.ringFull = true;
            }
        }
    }

    /*
    ** Копия хвоста кольца без замка: при сбое запись в кольцо уже не идёт (или идёт в упавшем потоке),
    ** а худшее — оборванная первая строка. Индексы читаются один раз.
    **
    ** A copy of the ring tail without the lock: on a crash the ring is no longer written (or is written in the crashed thread),
    ** and the worst case is a cut first line. The indices are read once.
    */
    std::size_t copyRecent(char* buffer, std::size_t size) noexcept
    {
        const internal::LogState& state = internal::logState();
        std::size_t head = state.ringHead;
        std::size_t total = head;
        std::size_t count = 0;
        std::size_t start = 0;

        if (state.ringFull)
            total = internal::RING_BYTES;
        count = std::min(total, size);
        start = (head + internal::RING_BYTES - count) % internal::RING_BYTES;
        for (std::size_t index = 0; index < count; ++index)
            buffer[index] = state.ring[(start + index) % internal::RING_BYTES];
        return count;
    }
}
