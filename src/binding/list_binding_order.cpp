#include "gran/binding/internal/list_binding.hpp"
#include "gran/data/internal/list_notify.hpp"

#include <utility>

namespace gran::binding
{
    void ListBinding::moved(std::size_t from, std::size_t count, std::size_t to)
    {
        reordered(data::internal::moveMapping(_model.size(), from, count, to));
    }

    /*
    ** Обычный список переставляет готовые копии (фокус, текст полей и прокрутка внутри записей
    ** сохраняются). Виртуальный строит то же окно заново: его копии — лишь часть записей.
    **
    ** An ordinary list rearranges the ready copies (the focus, the field text and the scrolling inside the records
    ** are kept). A virtual one builds the same window anew: its copies are only a part of the records.
    */
    void ListBinding::reordered(const std::vector<std::size_t>& oldToNew)
    {
        std::size_t first = _first;
        std::size_t last = _first + _items.size();

        if (!_virtual && oldToNew.size() == _items.size() && _first == 0)
        {
            permute(oldToNew);
            // Копии унесли с собой отметки, рассчитанные выделением до перестановки (см. ListBinding::inserted)
            // The copies carried away the marks computed by the selection before the permutation (see ListBinding::inserted)
            selectionChanged();
            return;
        }
        while (!_items.empty())
            dematerialize(_items.size() - 1);
        _first = first;
        reshapeWindow(first, std::min(last, _model.size()));
        applySpacers();
    }

    // Подписчики выделения узнают об изменении целиком — обновляются отметки всех копий окна / The selection subscribers learn about the change as a whole — the marks of all window copies are updated
    void ListBinding::selectionChanged()
    {
        for (std::size_t local = 0; local < _items.size(); ++local)
            markSelection(_items[local], _first + local);
    }

    // Копии занимают в детях контейнера непрерывный участок, который переставляется на месте / The copies occupy a contiguous run among the container children, which is rearranged in place
    void ListBinding::permute(const std::vector<std::size_t>& oldToNew)
    {
        std::size_t start = childPosition(0);
        std::size_t count = _items.size();
        std::vector<std::unique_ptr<ui::Node>> nodes(count);
        std::vector<BoundItem> items(count);

        for (std::size_t old = 0; old < count; ++old)
        {
            nodes[oldToNew[old]] = std::move(_container.children[start + old]);
            items[oldToNew[old]] = std::move(_items[old]);
        }
        for (std::size_t place = 0; place < count; ++place)
            _container.children[start + place] = std::move(nodes[place]);
        _items = std::move(items);
        ui::markDirty(_container, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }

    // Как состояния ввода: смена бита требует пересчёта стиля копии / As the input states: a bit change requires a style recompute of the copy
    void ListBinding::markSelection(const BoundItem& item, std::size_t record)
    {
        std::uint16_t bit = style::stateBit(style::State::SELECTED);
        std::uint16_t mask = static_cast<std::uint16_t>(item.root->stateMask & ~bit);

        if (_selection != nullptr && _selection->isSelected(record))
            mask = static_cast<std::uint16_t>(mask | bit);
        if (mask == item.root->stateMask)
            return;
        item.root->stateMask = mask;
        ui::markDirty(*item.root, ui::DIRTY_STYLE | ui::DIRTY_PAINT);
    }
}
