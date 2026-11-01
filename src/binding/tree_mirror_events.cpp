#include "gran/binding/internal/expression_binding.hpp"

namespace gran::binding
{
    // Правка дерева только помечает зеркало: тысяча вставок подряд — одна перестройка перед вычислениями / A tree edit only marks the mirror: a thousand insertions in a row are one rebuild before the evaluations
    void TreeMirror::inserted(data::NodeId, std::size_t, std::size_t)
    {
        dirty = true;
        ++version;
    }

    void TreeMirror::removed(data::NodeId, std::size_t, std::size_t)
    {
        dirty = true;
        ++version;
    }

    void TreeMirror::moved(data::NodeId, data::NodeId, std::size_t)
    {
        dirty = true;
        ++version;
    }

    void TreeMirror::changed(data::NodeId)
    {
        dirty = true;
        ++version;
    }
}
