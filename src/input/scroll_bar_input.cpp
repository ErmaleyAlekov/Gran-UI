#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Координата точки вдоль оси полосы / The point coordinate along the bar axis
        float along(Vec2 point, bool vertical) noexcept
        {
            if (vertical)
                return point.y;
            return point.x;
        }

        // Смещение прокрутки по оси; предел наложит компоновка / The scroll offset along the axis; the layout applies the limit
        void setOffset(ui::Node& scroll, bool vertical, float value)
        {
            if (vertical)
                scroll.layout.scrollOffset.y = value;
            else
                scroll.layout.scrollOffset.x = value;
            ui::markDirty(scroll, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }
    }

    /*
    ** Полоса лежит поверх содержимого, поэтому под мышью — узел содержимого: вверх от него ищется первый
    ** Scroll или TextArea, чья полоса (вертикальная, затем горизонтальная) содержит точку.
    **
    ** The bar lies over the content, so the node under the mouse is a content node: up from it the first
    ** Scroll or TextArea is searched whose bar (the vertical one, then the horizontal one) contains the point.
    */
    ui::Node* scrollBarAt(RouterState& state, Vec2 point, bool& vertical)
    {
        ui::ScrollBar bar;

        for (ui::Node* node = pickNode(state, point); node != nullptr; node = node->parent)
        {
            Vec2 local = ui::pointerToNode(*node, point);
            for (int axis = 0; axis < 2; ++axis)
            {
                vertical = axis == 0;
                if (ui::scrollBar(*node, vertical, bar) && rectContains(bar.track, local))
                    return node;
            }
        }
        return nullptr;
    }

    /*
    ** Нажатие на бегунок захватывает его (место захвата запоминается, бегунок не прыгает под указатель),
    ** нажатие на полосу мимо бегунка листает на видимую часть в сторону точки — как полосы Windows.
    ** События содержимому не идут: полоса — часть прокрутки, а не того, что под ней.
    **
    ** A press on the thumb grabs it (the grab place is remembered, the thumb does not jump under the pointer),
    ** a press on the bar beside the thumb pages by the visible part towards the point — as the Windows bars do.
    ** The content gets no events: the bar is a part of the scroll, not of what lies under it.
    */
    bool pressScrollBar(RouterState& state, const platform::Event& event)
    {
        bool vertical = true;
        ui::ScrollBar bar;
        ui::Node* scroll = scrollBarAt(state, event.position, vertical);

        if (scroll == nullptr || !ui::scrollBar(*scroll, vertical, bar))
            return false;
        float point = along(ui::pointerToNode(*scroll, event.position), vertical);
        float thumbStart = along(Vec2{bar.thumb.x, bar.thumb.y}, vertical);
        float thumbLength = bar.thumb.height;
        float current = along(scroll->layout.scrollOffset, vertical);
        if (!vertical)
            thumbLength = bar.thumb.width;
        if (point < thumbStart)
            setOffset(*scroll, vertical, current - bar.view);
        else if (point >= thumbStart + thumbLength)
            setOffset(*scroll, vertical, current + bar.view);
        else
        {
            state.scrollDrag = scroll;
            state.scrollVertical = vertical;
            state.scrollGrab = point - thumbStart;
        }
        return true;
    }

    // Бегунок идёт за указателем: место бегунка → смещение прокрутки / The thumb follows the pointer: the thumb place → the scroll offset
    void dragScrollBar(RouterState& state, Vec2 point)
    {
        ui::Node* scroll = state.scrollDrag;
        ui::ScrollBar bar;

        if (scroll == nullptr || !ui::scrollBar(*scroll, state.scrollVertical, bar))
            return;
        float trackStart = along(Vec2{bar.track.x, bar.track.y}, state.scrollVertical);
        float place = along(ui::pointerToNode(*scroll, point), state.scrollVertical) - state.scrollGrab - trackStart;
        setOffset(*scroll, state.scrollVertical, ui::scrollForThumb(bar, place));
    }
}
