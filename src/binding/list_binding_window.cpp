#include "gran/binding/internal/list_binding.hpp"

#include <algorithm>
#include <cmath>

namespace gran::binding
{
    namespace
    {
        // Сколько записей держать сверх видимых с каждой стороны (плавная прокрутка без пустот) / How many records to keep beyond the visible ones on each side (smooth scrolling without gaps)
        const std::size_t OVERSCAN = 8;

        // Наименьшее окно до измерения размеров / The smallest window before the sizes are measured
        const std::size_t INITIAL_WINDOW = 32;

        // Отрезок главной оси в координатах окна / A main axis segment in window coordinates
        struct Span
        {
            float start;
            float size;
        };

        // Начало и размер блока содержимого узла по главной оси (рамка без обводки и отступов) / The start and size of the node content box along the main axis (the frame without the border and padding)
        Span contentSpan(const ui::Node& node, bool horizontal) noexcept
        {
            const style::ComputedStyle& style = node.computed;
            const Rect& frame = node.layout.frame;

            if (horizontal)
                return Span{frame.x + style.borderWidth.left + style.padding.left, frame.width
                    - style.borderWidth.left - style.borderWidth.right - style.padding.left - style.padding.right};
            return Span{frame.y + style.borderWidth.top + style.padding.top, frame.height
                - style.borderWidth.top - style.borderWidth.bottom - style.padding.top - style.padding.bottom};
        }

        // Видимая область: ближайшая прокрутка вверх по дереву, без неё — корень (окно) / The visible area: the nearest scroll up the tree, without one — the root (the window)
        Span viewportSpan(const ui::Node& container, bool horizontal) noexcept
        {
            const ui::Node* current = container.parent;

            while (current != nullptr && current->kind != ui::ElementKind::SCROLL && current->parent != nullptr)
                current = current->parent;
            if (current == nullptr)
                current = &container;
            return contentSpan(*current, horizontal);
        }
    }

    /*
    ** Сначала копии окна измеряются (их размеры заменяют оценку в индексе), и если видимая запись
    ** сдвинулась в содержимом — прокрутка сдвигается на ту же величину (якорь, anchorShift).
    ** Затем окно — записи, на которые приходится видимая область, плюс OVERSCAN с каждой стороны;
    ** номер записи по координате даёт индекс размеров, поэтому строки разной высоты не дают пустот.
    **
    ** First the window copies are measured (their sizes replace the estimate in the index), and if the visible record
    ** moved in the content — the scroll moves by the same amount (the anchor, anchorShift).
    ** Then the window is the records the visible area falls on, plus OVERSCAN on each side;
    ** the record index by a coordinate comes from the size index, so rows of different heights leave no gaps.
    */
    bool ListBinding::updateWindow()
    {
        bool horizontal = _container.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);
        if (!_virtual)
            return false;
        measureItems(horizontal);
        if (_extents.estimate() <= 0.0f)
            return false;
        float start = contentSpan(_container, horizontal).start;
        Span view = viewportSpan(_container, horizontal);
        float shift = anchorShift(start, view.start, view.start + view.size, horizontal);
        bool anchored = anchorScroll(shift, horizontal);
        double offset = static_cast<double>(view.start - start + shift);
        _anchor = _extents.indexAt(offset);
        _anchorAt = _extents.offsetOf(_anchor);
        std::size_t count = _model.size();
        std::size_t first = _extents.indexAt(offset);
        std::size_t last = std::min(count, _extents.indexAt(offset + static_cast<double>(view.size)) + 1 + OVERSCAN);
        first -= std::min(first, OVERSCAN);
        last = std::max(first, last);
        _capacity = std::max(INITIAL_WINDOW, last - first);
        bool changed = reshapeWindow(first, last) || anchored;
        if (last >= count)
            changed = fetchMore() || changed;
        return applySpacers() || changed;
    }

    // Пересекающееся окно меняется по краям (копии в середине живут), непересекающееся — строится заново
    // An overlapping window changes at its edges (the copies in the middle live on), a disjoint one is built anew
    bool ListBinding::reshapeWindow(std::size_t first, std::size_t last)
    {
        bool changed = false;

        if (last <= _first || first >= _first + _items.size())
        {
            changed = !_items.empty() || first < last;
            while (!_items.empty())
                dematerialize(_items.size() - 1);
            _first = first;
        }
        for (; _first < first && !_items.empty(); ++_first, changed = true)
            dematerialize(0);
        for (; _first + _items.size() > last && !_items.empty(); changed = true)
            dematerialize(_items.size() - 1);
        for (; _first > first; changed = true)
            materialize(--_first);
        for (; _first + _items.size() < last; changed = true)
            materialize(_first + _items.size());
        return changed;
    }
}
