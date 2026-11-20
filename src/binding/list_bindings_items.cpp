#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    /*
    ** Для доступности и кода приложения, которым нужен список целиком, а не запись под узлом (itemOf): число записей,
    ** режим выделения, текущая запись.
    **
    ** For accessibility and the application code that need the list as a whole rather than the record under a node (itemOf):
    ** the record count, the selection mode, the current record.
    */
    ItemRef ListBindings::listOf(const ui::Node& container) const noexcept
    {
        ListBinding* binding = bindingOf(container);
        ItemRef result;

        if (binding == nullptr)
            return result;
        result.model = &binding->model();
        result.selection = binding->selection();
        result.index = data::NO_ROW;
        if (result.selection != nullptr)
            result.index = result.selection->current();
        return result;
    }

    ui::Node* ListBindings::itemNode(const ui::Node& container, std::size_t index) const noexcept
    {
        ListBinding* binding = bindingOf(container);

        if (binding == nullptr)
            return nullptr;
        return binding->itemAt(index);
    }

    // Копия появится после компоновки: виртуальный список строит окно вокруг новой прокрутки / The copy appears after the layout: a virtual list builds the window around the new scroll
    bool ListBindings::revealItem(const ui::Node& container, std::size_t index)
    {
        ListBinding* binding = bindingOf(container);

        if (binding == nullptr || index >= binding->model().size())
            return false;
        binding->reveal(index);
        return true;
    }

    std::uint64_t ListBindings::listVersion(const ui::Node& container) const noexcept
    {
        ListBinding* binding = bindingOf(container);

        if (binding == nullptr)
            return 0;
        return binding->version();
    }

    void ListBindings::itemAdded(const ui::Node& node) noexcept
    {
        if (_listener != nullptr)
            _listener->nodeAdded(node);
    }
}
