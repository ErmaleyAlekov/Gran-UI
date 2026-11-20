#include "gran/app/internal/access_items.hpp"

namespace gran::app
{
    AccessItems::AccessItems(const binding::ListBindings& lists) noexcept : _lists(lists)
    {
    }

    // itemOf ищет от узла вверх: у копии строки это сама строка / itemOf searches from the node upwards: for a row copy it is the row itself
    std::size_t AccessItems::itemIndex(const ui::Node& item) const noexcept
    {
        binding::ItemRef record = _lists.itemOf(item);

        if (record.model == nullptr)
            return a11y::NO_ITEM;
        return record.index;
    }

    // Размер источника, а не число копий: виртуальный список строит копии только около видимой области / The source size, not the copy count: a virtual list builds copies only near the visible area
    std::size_t AccessItems::itemCount(const ui::Node& list) const noexcept
    {
        binding::ItemRef source = _lists.listOf(list);

        if (source.model == nullptr)
            return 0;
        return source.model->size();
    }

    const ui::Node* AccessItems::itemAt(const ui::Node& list, std::size_t index) const noexcept
    {
        return _lists.itemNode(list, index);
    }

    std::uint64_t AccessItems::itemVersion(const ui::Node& list) const noexcept
    {
        return _lists.listVersion(list);
    }
}
