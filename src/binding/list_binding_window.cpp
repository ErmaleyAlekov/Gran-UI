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

        // Средний размер записи с промежутком по рамкам уже скомпонованных копий (0 — копий нет)
        // The average record size with the gap by the frames of the copies already laid out (0 — no copies)
        float measureExtent(const std::vector<BoundItem>& items, float spacing, bool horizontal) noexcept
        {
            if (items.empty())
                return 0.0f;
            const Rect& first = items.front().root->layout.frame;
            const Rect& last = items.back().root->layout.frame;
            float span = last.y + last.height - first.y;
            if (horizontal)
                span = last.x + last.width - first.x;
            return (span + spacing) / static_cast<float>(items.size());
        }
    }

    /*
    ** Окно — записи, попадающие в видимую область, плюс OVERSCAN с каждой стороны. Номер
    ** записи по координате — через средний размер (для записей примерно одной высоты точно).
    **
    ** The window is the records falling into the visible area, plus OVERSCAN on each side. The record
    ** index by a coordinate goes through the average size (exact for records of about the same height).
    */
    bool ListBinding::updateWindow()
    {
        bool horizontal = _container.computed.orientation == static_cast<int>(style::Orientation::HORIZONTAL);

        if (!_virtual)
            return false;
        float measured = measureExtent(_items, _container.computed.spacing, horizontal);
        if (measured > 0.0f)
            _extent = measured;
        if (_extent <= 0.0f)
            return false;
        Span view = viewportSpan(_container, horizontal);
        float offset = view.start - contentSpan(_container, horizontal).start;
        double from = std::floor(offset / _extent) - static_cast<double>(OVERSCAN);
        double to = std::ceil((offset + view.size) / _extent) + static_cast<double>(OVERSCAN);
        std::size_t count = _model.size();
        std::size_t first = std::min(count, static_cast<std::size_t>(std::max(0.0, from)));
        std::size_t last = std::max(first, std::min(count, static_cast<std::size_t>(std::max(0.0, to))));
        _capacity = std::max(INITIAL_WINDOW, last - first);
        bool changed = reshapeWindow(first, last);
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
