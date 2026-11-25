#include "gran/binding/internal/list_binding.hpp"

namespace gran::binding
{
    float ListBinding::rowStart(bool horizontal) const noexcept
    {
        const auto& style = _container.computed;
        const auto& frame = _container.layout.frame;
        float origin = horizontal ? frame.x + style.borderWidth.left + style.padding.left
                                  : frame.y + style.borderWidth.top + style.padding.top;
        for (const auto& child : _container.children)
        {
            if (child.get() == _lead)
                break;
            if (!ui::inFlow(*child))
                continue;
            const auto& bounds = child->layout.frame;
            origin = horizontal ? bounds.x + bounds.width + child->computed.margin.right
                                : bounds.y + bounds.height + child->computed.margin.bottom;
            origin += style.spacing;
        }
        return origin;
    }

    /*
    ** Видимая область — рамка ближайшего Scroll. Копия записи есть — по её рамке; записи нет (вне окна
    ** виртуального списка) — по индексу размеров от начала содержимого контейнера. Предел прокрутки
    ** накладывает компоновка, как и для колеса мыши.
    **
    ** The visible area is the frame of the nearest Scroll. The record copy exists — by its frame; there is no record (outside
    ** the virtual list window) — by the size index from the container content start. The scroll limit
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
        if (scroll == nullptr || (item == nullptr && _extents.estimate() <= 0.0f))
            return;
        if (item != nullptr)
        {
            top = item->layout.frame.y;
            bottom = top + item->layout.frame.height;
        }
        else
        {
            top = rowStart(false) + static_cast<float>(_extents.offsetOf(index));
            bottom = top + _extents.sizeAt(index) - _container.computed.spacing;
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
