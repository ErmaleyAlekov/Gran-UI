#include "gran/rhi/dx12/internal/dx12_internal.hpp"

#include <utility>

namespace gran::rhi::dx12
{
    // Объект переходит во владение очереди / The object passes into the queue ownership
    void DeferredReleases::push(ComRef<ID3D12Resource> resource, std::uint32_t slot, std::uint64_t fenceValue)
    {
        _entries.push_back(Entry{std::move(resource), slot, fenceValue});
    }

    /*
    ** Записи с пройденным значением забора удаляются (ComRef вызывает Release), остальные
    ** сохраняют порядок. Слоты отдаются вызывающему: дескриптор переписывает таблица текстур.
    **
    ** The entries with a passed fence value are removed (ComRef calls Release), the others
    ** keep their order. The slots are given to the caller: the texture table rewrites the descriptor.
    */
    void DeferredReleases::collect(std::uint64_t completedValue, std::vector<std::uint32_t>& freedSlots)
    {
        std::size_t kept = 0;

        for (std::size_t index = 0; index < _entries.size(); ++index)
        {
            if (_entries[index].fenceValue > completedValue)
            {
                if (kept != index)
                    _entries[kept] = std::move(_entries[index]);
                ++kept;
                continue;
            }
            if (_entries[index].slot != NO_SLOT)
                freedSlots.push_back(_entries[index].slot);
            _entries[index].resource.reset();
        }
        _entries.resize(kept);
    }
}
