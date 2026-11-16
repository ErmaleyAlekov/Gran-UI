#include "gran/binding/internal/list_binding.hpp"

#include <cmath>

namespace gran::binding
{
    namespace
    {
        // Начало и размер рамки копии по главной оси в координатах окна / The start and size of a copy frame along the main axis in window coordinates
        struct Edge
        {
            float start;
            float size;
        };

        Edge copyEdge(const ui::Node& copy, bool horizontal) noexcept
        {
            const Rect& frame = copy.layout.frame;

            if (horizontal)
                return Edge{frame.x, frame.width};
            return Edge{frame.y, frame.height};
        }
    }

    /*
    ** Размер записи — рамка копии плюс промежуток стека после неё (так сумма размеров записей
    ** перед окном — ровно место от начала содержимого до первой копии). Скрытая копия (visible
    ** ложно) не занимает места и не даёт промежутка — её размер ноль.
    **
    ** A record size is the copy frame plus the stack gap after it (so the sum of the record sizes
    ** before the window is exactly the room from the content start to the first copy). A hidden copy (visible
    ** is false) takes no room and gives no gap — its size is zero.
    */
    void ListBinding::measureItems(bool horizontal)
    {
        float gap = _container.computed.spacing;

        for (std::size_t local = 0; local < _items.size(); ++local)
        {
            const ui::Node& root = *_items[local].root;
            float size = root.layout.frame.height;

            if (horizontal)
                size = root.layout.frame.width;
            if (!root.visible)
                size = -gap;
            _extents.set(_first + local, size + gap);
        }
    }

    /*
    ** Прежний якорь берётся, даже если его копия ушла за край видимой области или из окна: место по индексу
    ** определено для любой записи, а отбросив якорь, потеряли бы сдвиг от копий над ним, оказавшихся меньше
    ** или больше оценки. Якоря нет (первый проход, правка модели) — первая видимая копия; копия ниже
    ** видимой области якорем не бывает: её место не держит видимые записи.
    **
    ** The previous anchor is taken even if its copy left the visible area or the window: the place by the index
    ** is defined for any record, and dropping the anchor would lose the shift from the copies above it that turned out
    ** smaller or larger than the estimate. No anchor (the first pass, a model edit) — the first visible copy; a copy below
    ** the visible area is never the anchor: its place does not hold the visible records.
    */
    float ListBinding::anchorShift(float start, float from, float to, bool horizontal) const
    {
        // Начало списка видно (прокрутили к самому верху): записей над окном нет, первая стоит у начала —
        // держать нечего, а старый якорь утянул бы прокрутку обратно вниз (ревью Т-60)
        // The list start is visible (scrolled to the very top): there are no records above the window, the first one stands
        // at the start — nothing to hold, and the old anchor would pull the scroll back down (review T-60)
        if (from <= start)
            return 0.0f;
        if (_anchor < _extents.size())
            return static_cast<float>(_extents.offsetOf(_anchor) - _anchorAt);
        for (std::size_t local = 0; local < _items.size(); ++local)
        {
            Edge edge = copyEdge(*_items[local].root, horizontal);
            if (!_items[local].root->visible || edge.start + edge.size <= from)
                continue;
            if (edge.start >= to)
                return 0.0f;
            return static_cast<float>(_extents.offsetOf(_first + local) - static_cast<double>(edge.start - start));
        }
        return 0.0f;
    }

    /*
    ** Сдвиг меньше сотой пикселя — не изменение (накопленная погрешность сумм). Предел прокрутки
    ** наложит следующая компоновка, как для колеса мыши.
    **
    ** A shift under a hundredth of a pixel is not a change (the accumulated error of the sums). The scroll limit
    ** is applied by the next layout, as for the mouse wheel.
    */
    bool ListBinding::anchorScroll(float shift, bool horizontal)
    {
        ui::Node* scroll = _container.parent;

        if (!(std::fabs(shift) >= 0.01f))
            return false;
        while (scroll != nullptr && scroll->kind != ui::ElementKind::SCROLL)
            scroll = scroll->parent;
        if (scroll == nullptr)
            return false;
        if (horizontal)
            scroll->layout.scrollOffset.x += shift;
        else
            scroll->layout.scrollOffset.y += shift;
        ui::markDirty(*scroll, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        return true;
    }
}
