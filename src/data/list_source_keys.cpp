#include "gran/data/list_source.hpp"

namespace gran::data
{
    // Плоский список и прокси ключей не дают: выделение работает по номерам, как раньше
    // A flat list and proxies give no keys: the selection works by indices, as before
    bool ListSource::hasRowKeys() const noexcept
    {
        return false;
    }

    std::uint64_t ListSource::rowKey(std::size_t index) const
    {
        return static_cast<std::uint64_t>(index);
    }
}
