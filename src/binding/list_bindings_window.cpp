#include "gran/binding/internal/expression_binding.hpp"
#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    /*
    ** Обход по номеру, а не итератором: смена окна удаляет копии вместе с вложенными привязками
    ** (forget сдвигает вектор). Пропущенная из-за сдвига привязка догонит на следующем проходе.
    **
    ** A walk by index, not by an iterator: changing a window removes copies together with their nested bindings
    ** (forget shifts the vector). A binding skipped because of the shift catches up on the next pass.
    */
    bool ListBindings::updateVirtual()
    {
        bool changed = false;

        for (std::size_t index = 0; index < _bindings.size(); ++index)
            changed = _bindings[index]->updateWindow() || changed;
        return changed;
    }

    bool ListBindings::updateExpressions()
    {
        return _expressions->update();
    }
}
