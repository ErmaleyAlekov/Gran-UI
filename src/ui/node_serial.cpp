#include "gran/ui/node.hpp"

#include <atomic>

namespace gran::ui
{
    /*
    ** Атомарный счётчик: узлы создают и фоновые задачи (разметка, загрузка вне потока интерфейса). 64 бита
    ** не переполнятся за время жизни процесса даже при миллионах узлов в секунду.
    **
    ** An atomic counter: background tasks create nodes too (markup, loading off the interface thread). 64 bits
    ** do not overflow during the process lifetime even at millions of nodes per second.
    */
    std::uint64_t nextNodeSerial() noexcept
    {
        static std::atomic<std::uint64_t> counter{0};

        return ++counter;
    }
}
