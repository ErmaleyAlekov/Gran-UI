#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    /*
    ** Видимая область — рамка ближайшего Scroll. Копия записи есть — по её рамке; записи нет (вне окна
    ** виртуального списка) — по среднему размеру записи от начала контейнера. Предел прокрутки
    ** накладывает компоновка, как и для колеса мыши.
    **
    ** The visible area is the frame of the nearest Scroll. The record copy exists — by its frame; there is no record (outside
    ** the virtual list window) — by the average record size from the container start. The scroll limit
    ** is applied by the layout, as for the mouse wheel.
    */
    void ListBinding::reveal(std::size_t index)
    {
        ui::Node* scroll = _container.parent;
        ui::Node* item = itemAt(index);
        float top = 0.0f;
        float bottom = 0.0f;

        while (scroll != nullptr && scroll->kind != ui::ElementKind::SCROLL)
            scroll = scroll->parent;
        if (scroll == nullptr || (item == nullptr && _extent <= 0.0f))
            return;
        top = _container.layout.frame.y + static_cast<float>(index) * _extent;
        bottom = top + _extent;
        if (item != nullptr)
        {
            top = item->layout.frame.y;
            bottom = top + item->layout.frame.height;
        }
        const Rect& view = scroll->layout.frame;
        float shift = 0.0f;
        if (top < view.y)
            shift = top - view.y;
        else if (bottom > view.y + view.height)
            shift = bottom - view.y - view.height;
        if (shift == 0.0f)
            return;
        scroll->layout.scrollOffset.y += shift;
        ui::markDirty(*scroll, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
    }
}
