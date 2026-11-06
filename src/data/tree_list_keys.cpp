#include "gran/data/tree_list.hpp"

namespace gran::data
{
    // Ключ строки — её узел: номер узла не меняется при свёртке, раскрытии, фильтре и сортировке
    // The row key is its node: the node id does not change on collapse, expansion, filter and sorting
    bool TreeList::hasRowKeys() const noexcept
    {
        return true;
    }

    std::uint64_t TreeList::rowKey(std::size_t index) const
    {
        return static_cast<std::uint64_t>(node(index));
    }
}
