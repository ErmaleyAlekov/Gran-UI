#include "gran/binding/internal/list_binding.hpp"

#include <algorithm>

namespace gran::binding
{
    namespace
    {
        // Шаг PageUp/PageDown в записях / The PageUp/PageDown step in records
        constexpr std::size_t PAGE = 10;

        // Следующая текущая запись по клавише; без текущей — первая / The next current record by the key; without a current one — the first
        std::size_t nextRow(std::size_t current, ListKey key, std::size_t size) noexcept
        {
            if (current == data::NO_ROW || current >= size || key == ListKey::HOME)
                return 0;
            if (key == ListKey::END)
                return size - 1;
            if (key == ListKey::UP && current > 0)
                return current - 1;
            if (key == ListKey::DOWN && current + 1 < size)
                return current + 1;
            if (key == ListKey::PAGE_UP)
                return current - std::min(current, PAGE);
            if (key == ListKey::PAGE_DOWN)
                return std::min(size - 1, current + PAGE);
            return current;
        }

        /*
        ** Стрелки плоского дерева, как в проводнике: вправо — раскрыть, у раскрытого — к первому ребёнку;
        ** влево — свернуть, у свёрнутого — к родителю. target = NO_ROW — текущая не меняется.
        **
        ** The flat tree arrows, as in a file manager: right — expand, for an expanded one — to the first child;
        ** left — collapse, for a collapsed one — to the parent. target = NO_ROW — the current one does not change.
        */
        void treeKey(data::TreeList& rows, std::size_t current, ListKey key, std::size_t& target)
        {
            target = data::NO_ROW;
            if (current == data::NO_ROW || current >= rows.size())
                return;
            if (key == ListKey::RIGHT && rows.hasChildren(current) && !rows.expanded(current))
                rows.expand(current);
            else if (key == ListKey::RIGHT && rows.expanded(current))
                target = current + 1;
            else if (key == ListKey::LEFT && rows.expanded(current))
                rows.collapse(current);
            else if (key == ListKey::LEFT)
            {
                for (std::size_t row = current; row > 0 && target == data::NO_ROW; --row)
                {
                    if (rows.depth(row - 1) < rows.depth(current))
                        target = row - 1;
                }
            }
        }

        // Отметки клавишей: Ctrl+A — все, пробел — текущую (Ctrl+пробел — переключить); true — обработано / Marks by a key: Ctrl+A — all, space — the current one (Ctrl+space — toggle); true — handled
        bool markKey(data::SelectionModel& selection, ListKey key, bool control)
        {
            std::size_t current = selection.current();

            if (key == ListKey::SELECT_ALL)
                selection.selectAll();
            else if (key != ListKey::SPACE)
                return false;
            else if (control && current != data::NO_ROW)
                selection.toggle(current);
            else if (current != data::NO_ROW)
                selection.select(current);
            return true;
        }
    }

    // Запись — прямой ребёнок контейнера на пути от узла под мышью вверх / The record is the direct container child on the path up from the node under the mouse
    bool ListBindings::pressItem(const ui::Node& container, const ui::Node& hit, bool control, bool shift)
    {
        ListBinding* binding = bindingOf(container);

        if (binding == nullptr || binding->selection() == nullptr)
            return false;
        for (const ui::Node* current = &hit; current != nullptr && current != &container; current = current->parent)
        {
            if (current->parent != &container)
                continue;
            std::size_t index = binding->indexOf(*current);
            if (index == data::NO_ROW)
                return false;
            binding->selection()->activate(index, control, shift);
            return true;
        }
        return false;
    }

    bool ListBindings::keyItem(const ui::Node& container, ListKey key, bool control, bool shift)
    {
        ListBinding* binding = bindingOf(container);
        data::SelectionModel* selection = nullptr;
        std::size_t target = data::NO_ROW;

        if (binding != nullptr)
            selection = binding->selection();
        if (selection == nullptr || binding->model().size() == 0)
            return false;
        if (markKey(*selection, key, control))
            return true;
        data::TreeList* rows = dynamic_cast<data::TreeList*>(&binding->model());
        if ((key == ListKey::LEFT || key == ListKey::RIGHT) && rows == nullptr)
            return false;
        if (key == ListKey::LEFT || key == ListKey::RIGHT)
            treeKey(*rows, selection->current(), key, target);
        else
            target = nextRow(selection->current(), key, binding->model().size());
        if (target == data::NO_ROW || target >= binding->model().size())
            return true;
        if (control)
            selection->setCurrent(target);
        else
            selection->activate(target, false, shift);
        binding->reveal(target);
        return true;
    }
}
