#include "gran/capi/internal/bridge.hpp"

namespace gran::capi
{
    // Несуществующий номер проверит само дерево (ошибка из каталога) / A missing number is checked by the tree itself (an error from the catalog)
    data::NodeId treeNode(const data::TreeModel& tree, uint64_t node)
    {
        if (node == 0)
            return tree.root();
        return node;
    }

    uint64_t abiNode(const data::TreeModel& tree, data::NodeId node) noexcept
    {
        if (node == tree.root())
            return 0;
        return node;
    }
}
